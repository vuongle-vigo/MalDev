import asyncio
import os
import shutil
import struct
import subprocess
import time
from pathlib import Path

from aiohttp import web, WSMsgType
import re


HOST = "0.0.0.0"
PORT = 8080

# Có thể set env:
#   Linux:   export FFMPEG_EXE=/usr/bin/ffmpeg
#   Windows: set FFMPEG_EXE=C:\FFMPEG\bin\ffmpeg.exe
FFMPEG_EXE = os.environ.get("FFMPEG_EXE") or shutil.which("ffmpeg") or r"C:\FFMPEG\bin\ffmpeg.exe"

HLS_DIR = Path("hls")
RECORDINGS_DIR = Path("recordings")
HEADER_SIZE = 24

# C++ header:
# struct H264WsHeader {
#     uint32_t magic;
#     uint32_t version;
#     uint32_t payloadSize;
#     uint64_t pts;
#     uint32_t flags;
# };
HEADER_STRUCT = struct.Struct("<IIIQI")
MAGIC_H264 = 0x34363248  # 'H264' little-endian


class StreamState:
    def __init__(self, agent_id: str):
        self.agent_id = agent_id
        self.hls_dir = HLS_DIR / agent_id
        self.recordings_dir = RECORDINGS_DIR / agent_id

        self.ffmpeg = None
        self.lock = asyncio.Lock()
        self.packet_count = 0
        self.total_bytes = 0
        self.last_packet_time = 0.0

        # Queue giúp websocket không bị block bởi stdin.flush/write của FFmpeg.
        self.write_queue = asyncio.Queue(maxsize=300)
        self.writer_task = None
        self.stderr_task = None
        # Recording: write raw h264 packets to a temp file, convert to mp4 on stop
        self.save_proc = None
        self.save_path = None
        self.save_tmp_path = None
        self.save_handle = None

        # viewers count and idle stop task
        self.viewers = 0
        self._stop_task = None

    def ffmpeg_running(self):
        return self.ffmpeg is not None and self.ffmpeg.poll() is None

    async def start_ffmpeg(self):
        async with self.lock:
            if self.ffmpeg_running():
                return

            ffmpeg_path = Path(FFMPEG_EXE)
            if not ffmpeg_path.exists() and shutil.which(str(FFMPEG_EXE)) is None:
                raise FileNotFoundError(f"ffmpeg not found: {FFMPEG_EXE}")

            if self.hls_dir.exists():
                shutil.rmtree(self.hls_dir, ignore_errors=True)
            self.hls_dir.mkdir(parents=True, exist_ok=True)

            # Không dùng append_list vì dễ giữ playlist cũ / làm player đọc nhầm trạng thái cũ.
            # hls_time quá nhỏ như 0.3 đôi khi làm browser request dày và lag hơn.
            cmd = [
                str(FFMPEG_EXE),

                "-hide_banner",
                "-loglevel", "warning",

                "-fflags", "+genpts+nobuffer",
                "-flags", "low_delay",
                "-use_wallclock_as_timestamps", "1",

                "-f", "h264",
                "-i", "pipe:0",

                "-an",

                "-c:v", "libx264",
                "-preset", "ultrafast",
                "-tune", "zerolatency",
                "-pix_fmt", "yuv420p",
                "-profile:v", "baseline",
                "-level", "4.0",

                # GOP ngắn để HLS có keyframe thường xuyên.
                "-x264-params", "keyint=15:min-keyint=15:scenecut=0:bframes=0",
                "-r", "30",
                "-g", "15",

                "-f", "hls",
                "-hls_time", "0.5",
                "-hls_list_size", "12",
                "-hls_delete_threshold", "12",
                "-hls_flags", "delete_segments+omit_endlist+independent_segments+temp_file",
                "-hls_segment_filename", str(self.hls_dir / "seg_%06d.ts"),
                "-hls_segment_type", "mpegts",

                str(self.hls_dir / "stream.m3u8"),
            ]

            print("[FFMPEG] starting:")
            print(" ".join(cmd))

            self.ffmpeg = subprocess.Popen(
                cmd,
                stdin=subprocess.PIPE,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE,
                bufsize=0,
            )

            self.packet_count = 0
            self.total_bytes = 0
            self.last_packet_time = 0.0

            # Xóa queue cũ nếu có packet tồn.
            while not self.write_queue.empty():
                try:
                    self.write_queue.get_nowait()
                    self.write_queue.task_done()
                except Exception:
                    break

            self.writer_task = asyncio.create_task(self._ffmpeg_writer())
            self.stderr_task = asyncio.create_task(self._read_ffmpeg_stderr())

    async def _read_ffmpeg_stderr(self):
        if not self.ffmpeg or not self.ffmpeg.stderr:
            return

        loop = asyncio.get_running_loop()

        while True:
            try:
                line = await loop.run_in_executor(None, self.ffmpeg.stderr.readline)
            except Exception as e:
                print("[FFMPEG STDERR ERROR]", e)
                break

            if not line:
                break

            text = line.decode("utf-8", errors="ignore").strip()
            if text:
                print("[FFMPEG]", text)

    def _write_stdin_sync(self, payload: bytes):
        if not self.ffmpeg or not self.ffmpeg.stdin:
            return False
        if self.ffmpeg.poll() is not None:
            return False

        self.ffmpeg.stdin.write(payload)
        # flush từng packet giúp delay thấp, nhưng đã đưa sang executor để không block event loop.
        self.ffmpeg.stdin.flush()
        return True

    async def _ffmpeg_writer(self):
        loop = asyncio.get_running_loop()

        while True:
            payload = await self.write_queue.get()

            try:
                if payload is None:
                    return

                ok = await loop.run_in_executor(None, self._write_stdin_sync, payload)

                if not ok:
                    print("[FFMPEG] not running while writing")
                    async with self.lock:
                        self.ffmpeg = None
                    return

            except BrokenPipeError:
                print("[FFMPEG] broken pipe")
                async with self.lock:
                    self.ffmpeg = None
                return

            except Exception as e:
                print("[FFMPEG WRITE ERROR]", e)
                async with self.lock:
                    self.ffmpeg = None
                return

            finally:
                self.write_queue.task_done()

    async def stop_ffmpeg(self):
        async with self.lock:
            if self.writer_task and not self.writer_task.done():
                try:
                    self.write_queue.put_nowait(None)
                except Exception:
                    pass

            if self.ffmpeg:
                try:
                    if self.ffmpeg.stdin:
                        self.ffmpeg.stdin.close()
                except Exception:
                    pass

                try:
                    self.ffmpeg.terminate()
                    self.ffmpeg.wait(timeout=2)
                except Exception:
                    try:
                        self.ffmpeg.kill()
                    except Exception:
                        pass

                self.ffmpeg = None

    def is_saving(self):
        return self.save_handle is not None

    def start_save(self, target_path: Path):
        # Start writing raw H264 packets to a temp file. Convert to mp4 on stop.
        if self.is_saving():
            return False

        tmp = target_path.with_suffix('.h264')
        try:
            tmp.parent.mkdir(parents=True, exist_ok=True)
            fh = open(tmp, 'wb')
        except Exception as e:
            print("[SAVE] cannot open tmp file:", e)
            return False

        self.save_tmp_path = str(tmp)
        self.save_path = str(target_path)
        self.save_handle = fh
        print("[SAVE] started ->", self.save_tmp_path)
        return True

    def stop_save(self):
        if not self.save_handle:
            return False

        try:
            try:
                self.save_handle.flush()
                self.save_handle.close()
            except Exception:
                pass

            tmp = self.save_tmp_path
            final = self.save_path

            # reset immediate state
            self.save_handle = None
            self.save_tmp_path = None
            self.save_path = None

            if not tmp or not final:
                return True

            # Convert raw h264 -> mp4 in background
            cmd = [
                str(FFMPEG_EXE),
                "-hide_banner",
                "-loglevel",
                "warning",
                "-y",
                "-f",
                "h264",
                "-i",
                str(tmp),
                "-c:v",
                "copy",
                str(final),
            ]

            print("[SAVE] converting:", " ".join(cmd))
            try:
                p = subprocess.Popen(cmd, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
            except Exception as e:
                print("[SAVE] conversion failed start:", e)
                return True

            # detach: run a thread to wait and then remove tmp
            def _wait_and_cleanup(proc, tmp_path):
                try:
                    proc.wait()
                except Exception:
                    pass
                try:
                    os.remove(tmp_path)
                except Exception:
                    pass

            import threading
            threading.Thread(target=_wait_and_cleanup, args=(p, tmp), daemon=True).start()

            return True
        finally:
            return True

    async def write_h264(self, payload: bytes):
        if not payload:
            return

        # Start ffmpeg only when there are viewers
        if self.viewers > 0:
            await self.start_ffmpeg()

        if not self.ffmpeg_running():
            # drop packets when ffmpeg not running
            return

        self.packet_count += 1
        self.total_bytes += len(payload)
        self.last_packet_time = time.time()

        # If recording active, write raw payload to temp file
        try:
            if self.save_handle:
                try:
                    self.save_handle.write(payload)
                except Exception:
                    pass
        except Exception:
            pass

        # Nếu browser/FFmpeg lag, queue đầy thì bỏ frame cũ để ưu tiên live mới.
        if self.write_queue.full():
            dropped = 0
            while self.write_queue.qsize() > 60:
                try:
                    self.write_queue.get_nowait()
                    self.write_queue.task_done()
                    dropped += 1
                except Exception:
                    break
            if dropped:
                print(f"[STREAM] dropped {dropped} old packets because writer queue was full")

        try:
            self.write_queue.put_nowait(payload)
        except asyncio.QueueFull:
            print("[STREAM] drop packet: writer queue full")

        if self.packet_count % 60 == 0:
            print(
                f"[STREAM] packets={self.packet_count}, "
                f"total={self.total_bytes / 1024:.1f} KB, "
                f"queue={self.write_queue.qsize()}"
            )


STATES = {}

def _sanitize_agent(agent: str) -> str:
    if not agent:
        return "default"
    # allow alnum, -, _ only
    if re.match(r"^[A-Za-z0-9_-]+$", agent):
        return agent
    raise web.HTTPBadRequest(text="Bad agent id")

def get_state(agent: str) -> StreamState:
    aid = _sanitize_agent(agent)
    s = STATES.get(aid)
    if s is None:
        s = StreamState(aid)
        STATES[aid] = s
    return s


async def agent_ws_handler(request):
    agent = request.match_info.get("agent") or "default"
    try:
        state = get_state(agent)
    except web.HTTPBadRequest as e:
        return web.Response(status=400, text=str(e))

    ws = web.WebSocketResponse(max_msg_size=0, heartbeat=15)
    await ws.prepare(request)

    print("[AGENT] connected")

    # ensure ffmpeg running if there are viewers
    try:
        if state.viewers > 0:
            await state.start_ffmpeg()
    except Exception as e:
        print("[FFMPEG] start failed:", e)
        await ws.close()
        return ws

    first_packet = True

    async for msg in ws:
        if msg.type == WSMsgType.BINARY:
            data = msg.data

            if len(data) < HEADER_SIZE:
                print("[AGENT] packet too small")
                continue

            magic, version, payload_size, pts, flags = HEADER_STRUCT.unpack(
                data[:HEADER_SIZE]
            )

            if magic != MAGIC_H264:
                print("[AGENT] bad magic:", hex(magic))
                continue

            payload = data[HEADER_SIZE:]

            if len(payload) != payload_size:
                print(
                    "[AGENT] bad payload size:",
                    len(payload),
                    "expected:",
                    payload_size,
                )
                continue

            if first_packet:
                first_packet = False
                print("[AGENT] first payload bytes:", payload[:16].hex(" "))

                if payload.startswith(b"\x00\x00\x00\x01") or payload.startswith(b"\x00\x00\x01"):
                    print("[AGENT] looks like Annex B H264")
                else:
                    print(
                        "[WARN] payload does not start with Annex B start code. "
                        "If ffmpeg cannot decode, your H264 output may be AVCC."
                    )

            # Không print từng packet vì sẽ làm terminal + event loop bị chậm.
            await state.write_h264(payload)

        elif msg.type == WSMsgType.TEXT:
            print("[AGENT] text:", msg.data)

        elif msg.type == WSMsgType.ERROR:
            print("[AGENT] ws error:", ws.exception())

    print("[AGENT] disconnected")
    return ws


async def index_handler(request):
    agent = request.match_info.get("agent") or "default"
    html = r"""
<!doctype html>
<html>
<head>
    <meta charset="utf-8">
    <title>C++ Agent Screen Stream</title>
    <style>
        body {
            margin: 0;
            background: #111;
            color: white;
            font-family: Arial, sans-serif;
        }

        .wrap {
            padding: 16px;
        }

        video {
            width: 100%;
            max-width: 1280px;
            background: black;
            border: 1px solid #333;
        }

        .status {
            margin-top: 12px;
            color: #aaa;
        }

        .ok {
            color: #68d391;
        }

        .bad {
            color: #fc8181;
        }

        code {
            background: #222;
            padding: 2px 5px;
            border-radius: 4px;
        }

        button {
            margin-top: 12px;
            padding: 8px 12px;
            cursor: pointer;
        }
    </style>
</head>
<body>
    <div class="wrap">
        <h2>C++ Agent Screen Stream</h2>

        <video id="video" controls autoplay muted playsinline></video>

        <div class="status" id="status">loading...</div>
        <div class="status" id="stats"></div>
        <div class="status">
            HLS URL: <code>/hls/__AGENT__/stream.m3u8</code>
        </div>

        <button onclick="restartPlayer()">Restart player</button>
            <button onclick="startSave()">Start Save</button>
            <button onclick="stopSave()">Stop Save</button>
            <div class="status" id="recordingStatus"></div>
    </div>

    <script src="https://cdn.jsdelivr.net/npm/hls.js@latest"></script>
<script>
    const video = document.getElementById("video");
    const statusEl = document.getElementById("status");

    let hls = null;
    let started = false;
    let retryTimer = null;

    function setStatus(text) {
        statusEl.textContent = text;
        console.log(text);
    }

    function hlsUrl() {
        return "/hls/__AGENT__/stream.m3u8?t=" + Date.now();
    }

    function isSafariNativeHls() {
        const ua = navigator.userAgent.toLowerCase();

        const isSafari =
            ua.includes("safari") &&
            !ua.includes("chrome") &&
            !ua.includes("chromium") &&
            !ua.includes("edg");

        return isSafari && video.canPlayType("application/vnd.apple.mpegurl");
    }

    function destroyPlayer() {
        if (hls) {
            try {
                hls.destroy();
            } catch (e) {}
            hls = null;
        }

        video.pause();
        video.removeAttribute("src");
        video.load();
    }

    async function waitForStreamReady() {
        while (true) {
            try {
                const res = await fetch("/health/__AGENT__?t=" + Date.now(), {
                    cache: "no-store"
                });

                const j = await res.json();

                setStatus(
                    "ffmpeg=" + j.ffmpeg_running +
                    ", hls=" + j.hls_exists +
                    ", packets=" + j.packets +
                    ", bytes=" + j.bytes +
                    ", queue=" + (j.queue_size ?? 0)
                );

                if (
                    j.ffmpeg_running &&
                    j.hls_exists &&
                    j.bytes > 0
                ) {
                    return;
                }
            } catch (e) {
                setStatus("waiting for server...");
            }

            await new Promise(r => setTimeout(r, 700));
        }
    }

    async function startPlayer() {
        if (started) return;
        started = true;

        clearTimeout(retryTimer);
        retryTimer = null;

        await waitForStreamReady();

        destroyPlayer();

        // Ưu tiên HLS.js cho Chrome/Edge/Firefox desktop
        if (window.Hls && Hls.isSupported() && !isSafariNativeHls()) {
            setStatus("starting HLS.js...");

            hls = new Hls({
                lowLatencyMode: false,

                // Cho buffer rộng hơn để tránh màn đen khi F5
                liveSyncDurationCount: 3,
                liveMaxLatencyDurationCount: 6,

                maxBufferLength: 10,
                backBufferLength: 0,
                maxBufferSize: 30 * 1000 * 1000,

                // Quan trọng: đừng fail quá nhanh khi segment vừa bị rotate
                manifestLoadingMaxRetry: 999,
                manifestLoadingRetryDelay: 500,
                levelLoadingMaxRetry: 999,
                levelLoadingRetryDelay: 500,
                fragLoadingMaxRetry: 999,
                fragLoadingRetryDelay: 500,

                enableWorker: true
            });

            hls.loadSource(hlsUrl());
            hls.attachMedia(video);

            hls.on(Hls.Events.MANIFEST_PARSED, async function () {
                setStatus("HLS.js manifest parsed");

                try {
                    video.muted = true;
                    await video.play();
                    setStatus("HLS.js playing");
                } catch (e) {
                    setStatus("click video to play: " + e.message);
                }
            });

            hls.on(Hls.Events.FRAG_BUFFERED, function () {
                if (video.paused) {
                    video.play().catch(() => {});
                }

                const w = video.videoWidth;
                const h = video.videoHeight;

                setStatus(
                    "playing via HLS.js, " +
                    "readyState=" + video.readyState +
                    ", size=" + w + "x" + h +
                    ", time=" + video.currentTime.toFixed(1)
                );
            });

            hls.on(Hls.Events.ERROR, function (event, data) {
                console.log("HLS error", data);

                setStatus("HLS error: " + data.type + " / " + data.details);

                if (!data.fatal) {
                    return;
                }

                if (data.type === Hls.ErrorTypes.NETWORK_ERROR) {
                    hls.startLoad();
                    return;
                }

                if (data.type === Hls.ErrorTypes.MEDIA_ERROR) {
                    hls.recoverMediaError();
                    return;
                }

                restartPlayerSoon();
            });

            return;
        }

        // Chỉ dùng native cho Safari thật sự
        if (isSafariNativeHls()) {
            setStatus("starting native Safari HLS...");

            video.src = hlsUrl();

            video.addEventListener("loadedmetadata", async function () {
                try {
                    video.muted = true;
                    await video.play();
                    setStatus("native Safari HLS playing");
                } catch (e) {
                    setStatus("native play failed: " + e.message);
                }
            }, { once: true });

            video.addEventListener("error", function () {
                setStatus("native HLS video error");
                restartPlayerSoon();
            });

            return;
        }

        setStatus("HLS is not supported in this browser");
    }

    function restartPlayerSoon() {
        started = false;

        clearTimeout(retryTimer);
        retryTimer = setTimeout(() => {
            startPlayer();
        }, 1000);
    }

    video.addEventListener("stalled", function () {
        setStatus("video stalled, restarting...");
        restartPlayerSoon();
    });

    video.addEventListener("waiting", function () {
        console.log("video waiting");
    });

    video.addEventListener("playing", function () {
        console.log("video playing");
    });

    startPlayer();

    // notify server this client is viewing
    window.addEventListener('load', function () {
        fetch('/view/__AGENT__/start', { method: 'POST' }).catch(() => {});
    });

    window.addEventListener('beforeunload', function () {
        try {
            navigator.sendBeacon('/view/__AGENT__/stop');
        } catch (e) {
            fetch('/view/__AGENT__/stop', { method: 'POST' }).catch(() => {});
        }
    });
    // Poll recording status periodically
    async function updateRecordingStatus() {
        try {
            const res = await fetch('/health/__AGENT__?t=' + Date.now(), { cache: 'no-store' });
            const j = await res.json();
            const el = document.getElementById('recordingStatus');
            if (j.saving) {
                el.innerHTML = 'Recording: <span class="ok">yes</span> — <a href="' + (j.save_file ? ('/recordings/__AGENT__/' + j.save_file.split('/').pop()) : '#') + '">download</a>';
            } else if (j.save_file) {
                const fname = j.save_file.split('/').pop();
                el.innerHTML = 'Last recording: <a href="/recordings/__AGENT__/' + fname + '">' + fname + '</a>';
            } else {
                el.textContent = 'Not recording';
            }
        } catch (e) {
            // ignore
        }
    }

    setInterval(updateRecordingStatus, 2000);

    async function startSave() {
        try {
            const res = await fetch('/save/__AGENT__/start', { method: 'POST' });
            const j = await res.json();
            if (!j.ok) {
                alert('Start save failed: ' + (j.error || 'unknown'));
            }
        } catch (e) {
            alert('Start save failed: ' + e.message);
        }
    }

    async function stopSave() {
        try {
            const res = await fetch('/save/__AGENT__/stop', { method: 'POST' });
            const j = await res.json();
            if (!j.ok) {
                alert('Stop save failed: ' + (j.error || 'unknown'));
            }
        } catch (e) {
            alert('Stop save failed: ' + e.message);
        }
    }
</script>
</body>
</html>
"""
    html = html.replace("__AGENT__", agent)
    return web.Response(
        text=html,
        content_type="text/html",
        headers={
            "Cache-Control": "no-store, no-cache, must-revalidate, max-age=0",
            "Pragma": "no-cache",
            "Expires": "0",
        },
    )


async def hls_handler(request):
    agent = request.match_info.get("agent") or "default"
    filename = request.match_info["filename"]

    try:
        state = get_state(agent)
    except web.HTTPBadRequest:
        return web.Response(status=400, text="Bad agent id")

    # Chặn path traversal.
    if "/" in filename or "\\" in filename or ".." in filename:
        return web.Response(status=400, text="Bad filename")

    path = state.hls_dir / filename

    if not path.exists():
        return web.Response(
            status=404,
            text="HLS file not ready",
            headers={
                "Cache-Control": "no-store, no-cache, must-revalidate, max-age=0",
                "Access-Control-Allow-Origin": "*",
            },
        )

    if filename.endswith(".m3u8"):
        content_type = "application/vnd.apple.mpegurl"
    elif filename.endswith(".ts"):
        content_type = "video/mp2t"
    else:
        content_type = "application/octet-stream"

    headers = {
        "Cache-Control": "no-store, no-cache, must-revalidate, max-age=0",
        "Pragma": "no-cache",
        "Expires": "0",
        "Access-Control-Allow-Origin": "*",
    }

    # Với manifest đọc trực tiếp giúp chắc chắn header no-cache có hiệu lực.
    if filename.endswith(".m3u8"):
        return web.Response(
            body=path.read_bytes(),
            content_type=content_type,
            headers=headers,
        )

    return web.FileResponse(path, headers=headers)


async def health_handler(request):
    agent = request.match_info.get("agent") or "default"
    try:
        state = get_state(agent)
    except web.HTTPBadRequest:
        return web.json_response({"ok": False, "error": "bad_agent"}, status=400)

    return web.json_response(
        {
            "ok": True,
            "packets": state.packet_count,
            "bytes": state.total_bytes,
            "queue": state.write_queue.qsize(),
            "last_packet_age": None if not state.last_packet_time else round(time.time() - state.last_packet_time, 2),
            "hls_exists": (state.hls_dir / "stream.m3u8").exists(),
            "ffmpeg_running": state.ffmpeg_running(),
            "saving": state.is_saving(),
            "save_file": state.save_path,
            "viewers": state.viewers,
        },
        headers={
            "Cache-Control": "no-store, no-cache, must-revalidate, max-age=0",
            "Access-Control-Allow-Origin": "*",
        },
    )


async def on_shutdown(app):
    # stop all agent states
    for s in list(STATES.values()):
        try:
            await s.stop_ffmpeg()
        except Exception:
            pass
        try:
            s.stop_save()
        except Exception:
            pass


async def save_start_handler(request):
    agent = request.match_info.get("agent") or "default"
    try:
        state = get_state(agent)
    except web.HTTPBadRequest:
        return web.json_response({"ok": False, "error": "bad_agent"}, status=400)

    if state.is_saving():
        return web.json_response({"ok": False, "error": "already_saving", "save_file": state.save_path})

    state.recordings_dir.mkdir(parents=True, exist_ok=True)
    filename = f"record_{int(time.time())}.mp4"
    target = state.recordings_dir / filename

    ok = state.start_save(target)
    if not ok:
        return web.json_response({"ok": False, "error": "start_failed"}, status=500)

    return web.json_response({"ok": True, "save_file": str(target)})


async def save_stop_handler(request):
    agent = request.match_info.get("agent") or "default"
    try:
        state = get_state(agent)
    except web.HTTPBadRequest:
        return web.json_response({"ok": False, "error": "bad_agent"}, status=400)

    if not state.is_saving():
        return web.json_response({"ok": False, "error": "not_saving"})

    state.stop_save()
    return web.json_response({"ok": True, "save_file": state.save_path})


async def view_start_handler(request):
    agent = request.match_info.get("agent") or "default"
    try:
        state = get_state(agent)
    except web.HTTPBadRequest:
        return web.json_response({"ok": False, "error": "bad_agent"}, status=400)

    state.viewers += 1
    # cancel scheduled stop
    if state._stop_task and not state._stop_task.done():
        state._stop_task.cancel()
        state._stop_task = None

    # start ffmpeg immediately
    try:
        await state.start_ffmpeg()
    except Exception as e:
        print("[VIEW] start_ffmpeg failed:", e)

    return web.json_response({"ok": True, "viewers": state.viewers})


async def view_stop_handler(request):
    agent = request.match_info.get("agent") or "default"
    try:
        state = get_state(agent)
    except web.HTTPBadRequest:
        return web.json_response({"ok": False, "error": "bad_agent"}, status=400)

    state.viewers = max(0, state.viewers - 1)

    # schedule stop after short delay if no viewers
    if state.viewers == 0:
        async def _delayed_stop(s: StreamState):
            await asyncio.sleep(5)
            if s.viewers == 0:
                try:
                    await s.stop_ffmpeg()
                except Exception:
                    pass

        state._stop_task = asyncio.create_task(_delayed_stop(state))

    return web.json_response({"ok": True, "viewers": state.viewers})


async def recordings_handler(request):
    agent = request.match_info.get("agent") or "default"
    filename = request.match_info["filename"]

    try:
        state = get_state(agent)
    except web.HTTPBadRequest:
        return web.Response(status=400, text="Bad agent id")

    # Chặn path traversal.
    if "/" in filename or "\\" in filename or ".." in filename:
        return web.Response(status=400, text="Bad filename")

    path = state.recordings_dir / filename
    if not path.exists():
        return web.Response(status=404, text="Recording not found")

    headers = {
        "Cache-Control": "no-store, no-cache, must-revalidate, max-age=0",
        "Pragma": "no-cache",
        "Expires": "0",
        "Access-Control-Allow-Origin": "*",
    }

    return web.FileResponse(path, headers=headers)


def create_app():
    app = web.Application()

    app.router.add_get("/", index_handler)
    app.router.add_get("/health/{agent}", health_handler)
    app.router.add_get("/agent/{agent}/stream", agent_ws_handler)
    app.router.add_get("/hls/{agent}/{filename}", hls_handler)
    app.router.add_post("/save/{agent}/start", save_start_handler)
    app.router.add_post("/save/{agent}/stop", save_stop_handler)
    app.router.add_get("/recordings/{agent}/{filename}", recordings_handler)

    # Optional: simple view page for an agent
    app.router.add_get("/view/{agent}", index_handler)
    app.router.add_post("/view/{agent}/start", view_start_handler)
    app.router.add_post("/view/{agent}/stop", view_stop_handler)

    app.on_shutdown.append(on_shutdown)

    return app


if __name__ == "__main__":
    app = create_app()

    print(f"HTTP server: http://127.0.0.1:{PORT}")
    print(f"Agent WS:    ws://127.0.0.1:{PORT}/agent/stream")
    print(f"FFmpeg:      {FFMPEG_EXE}")

    web.run_app(app, host=HOST, port=PORT)
