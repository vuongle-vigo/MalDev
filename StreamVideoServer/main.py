import asyncio
import shutil
import struct
import subprocess
from pathlib import Path

from aiohttp import web, WSMsgType


HOST = "0.0.0.0"
PORT = 8080

# Sửa path này theo vị trí ffmpeg.exe của bạn
FFMPEG_EXE = r"C:\FFMPEG\bin\ffmpeg.exe"

HLS_DIR = Path("hls")
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
    def __init__(self):
        self.ffmpeg = None
        self.lock = asyncio.Lock()
        self.packet_count = 0
        self.total_bytes = 0

    async def start_ffmpeg(self):
        async with self.lock:
            if self.ffmpeg and self.ffmpeg.poll() is None:
                return

            if not Path(FFMPEG_EXE).exists():
                raise FileNotFoundError(f"ffmpeg.exe not found: {FFMPEG_EXE}")

            if HLS_DIR.exists():
                shutil.rmtree(HLS_DIR)

            HLS_DIR.mkdir(parents=True, exist_ok=True)

            cmd = [
                FFMPEG_EXE,

                "-hide_banner",
                "-loglevel", "warning",

                "-fflags", "+genpts+nobuffer",
                "-flags", "low_delay",
                "-use_wallclock_as_timestamps", "1",

                "-r", "30",
                "-f", "h264",
                "-i", "pipe:0",

                "-an",

                "-c:v", "libx264",
                "-preset", "ultrafast",
                "-tune", "zerolatency",
                "-pix_fmt", "yuv420p",

                # giảm độ phức tạp encoder
                "-profile:v", "baseline",
                "-level", "4.0",
                "-x264-params", "keyint=15:min-keyint=15:scenecut=0:bframes=0",

                "-r", "30",
                "-g", "15",

                "-f", "hls",

                # segment ngắn hơn để giảm delay
                "-hls_time", "0.5",
                "-hls_list_size", "12",
                "-hls_delete_threshold", "12",
                "-hls_flags", "delete_segments+omit_endlist+independent_segments+temp_file",
                "-hls_segment_filename", str(HLS_DIR / "seg_%06d.ts"),
                "-hls_segment_type", "mpegts",

                str(HLS_DIR / "stream.m3u8"),
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

            asyncio.create_task(self._read_ffmpeg_stderr())

    async def _read_ffmpeg_stderr(self):
        if not self.ffmpeg or not self.ffmpeg.stderr:
            return

        while True:
            line = await asyncio.to_thread(self.ffmpeg.stderr.readline)
            if not line:
                break

            text = line.decode("utf-8", errors="ignore").strip()
            if text:
                print("[FFMPEG]", text)

    async def stop_ffmpeg(self):
        async with self.lock:
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

    async def write_h264(self, payload: bytes):
        if not payload:
            return

        await self.start_ffmpeg()

        async with self.lock:
            if not self.ffmpeg or not self.ffmpeg.stdin:
                return

            if self.ffmpeg.poll() is not None:
                print("[FFMPEG] process already exited, restarting")
                self.ffmpeg = None
                return

            try:
                self.ffmpeg.stdin.write(payload)
                self.ffmpeg.stdin.flush()

                self.packet_count += 1
                self.total_bytes += len(payload)

                if self.packet_count % 30 == 0:
                    print(
                        f"[STREAM] packets={self.packet_count}, "
                        f"total={self.total_bytes / 1024:.1f} KB"
                    )

            except BrokenPipeError:
                print("[FFMPEG] broken pipe")
                self.ffmpeg = None


state = StreamState()


async def agent_ws_handler(request):
    ws = web.WebSocketResponse(max_msg_size=0)
    await ws.prepare(request)

    print("[AGENT] connected")

    try:
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

                if payload.startswith(b"\x00\x00\x00\x01") or payload.startswith(
                    b"\x00\x00\x01"
                ):
                    print("[AGENT] looks like Annex B H264")
                else:
                    print(
                        "[WARN] payload does not start with Annex B start code. "
                        "If ffmpeg cannot decode, your H264 output may be AVCC."
                    )

            print(f"[AGENT] pts={pts} size={payload_size} flags={flags}")

            await state.write_h264(payload)

        elif msg.type == WSMsgType.TEXT:
            print("[AGENT] text:", msg.data)

        elif msg.type == WSMsgType.ERROR:
            print("[AGENT] ws error:", ws.exception())

    print("[AGENT] disconnected")
    return ws


async def index_handler(request):
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

        code {
            background: #222;
            padding: 2px 5px;
            border-radius: 4px;
        }
    </style>
</head>
<body>
    <div class="wrap">
        <h2>C++ Agent Screen Stream</h2>

        <video id="video" controls autoplay muted playsinline></video>

        <div class="status" id="status">loading...</div>
        <div class="status">
            HLS URL: <code>/hls/stream.m3u8</code>
        </div>
    </div>

    <script src="https://cdn.jsdelivr.net/npm/hls.js@latest"></script>
    <script>
        const video = document.getElementById("video");
        const statusEl = document.getElementById("status");
        const hlsUrl = "/hls/stream.m3u8";

        function setStatus(text) {
            statusEl.textContent = text;
            console.log(text);
        }

        if (video.canPlayType("application/vnd.apple.mpegurl")) {
            video.src = hlsUrl;
            video.addEventListener("loadedmetadata", function () {
                video.play();
            });
            setStatus("native HLS playback");
        } else if (window.Hls && Hls.isSupported()) {
            const hls = new Hls({
    lowLatencyMode: true,
    liveSyncDurationCount: 1,
    liveMaxLatencyDurationCount: 2,
    maxLiveSyncPlaybackRate: 1.5,
    maxBufferLength: 1,
    backBufferLength: 0,
    maxBufferSize: 10 * 1000 * 1000,
});

            hls.loadSource(hlsUrl);
            hls.attachMedia(video);

            hls.on(Hls.Events.MANIFEST_PARSED, function () {
                video.play();
                setStatus("HLS.js connected");
            });

            hls.on(Hls.Events.ERROR, function (event, data) {
                setStatus("HLS error: " + data.type + " / " + data.details);
                console.log(data);
            });
        } else {
            setStatus("HLS is not supported in this browser");
        }
    </script>
</body>
</html>
"""
    return web.Response(text=html, content_type="text/html")


async def hls_handler(request):
    filename = request.match_info["filename"]
    path = HLS_DIR / filename

    if not path.exists():
        return web.Response(status=404, text="HLS file not ready")

    if filename.endswith(".m3u8"):
        content_type = "application/vnd.apple.mpegurl"
    elif filename.endswith(".ts"):
        content_type = "video/mp2t"
    else:
        content_type = "application/octet-stream"

    return web.FileResponse(
        path,
        headers={
            "Cache-Control": "no-cache",
            "Access-Control-Allow-Origin": "*",
            "Content-Type": content_type,
        },
    )


async def health_handler(request):
    return web.json_response(
        {
            "ok": True,
            "packets": state.packet_count,
            "bytes": state.total_bytes,
            "hls_exists": (HLS_DIR / "stream.m3u8").exists(),
            "ffmpeg_running": state.ffmpeg is not None
            and state.ffmpeg.poll() is None,
        }
    )


async def on_shutdown(app):
    await state.stop_ffmpeg()


def create_app():
    app = web.Application()

    app.router.add_get("/", index_handler)
    app.router.add_get("/health", health_handler)
    app.router.add_get("/agent/stream", agent_ws_handler)
    app.router.add_get("/hls/{filename}", hls_handler)

    app.on_shutdown.append(on_shutdown)

    return app


if __name__ == "__main__":
    app = create_app()

    print(f"HTTP server: http://127.0.0.1:{PORT}")
    print(f"Agent WS:    ws://127.0.0.1:{PORT}/agent/stream")
    print(f"FFmpeg:      {FFMPEG_EXE}")

    web.run_app(app, host=HOST, port=PORT)