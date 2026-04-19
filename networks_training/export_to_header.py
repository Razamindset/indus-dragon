# embed_nnue.py
import numpy as np

data = np.fromfile("indus_dragon_v3.bin", dtype=np.int16)

with open("nnue_data.hpp", "w") as f:
    f.write("#pragma once\n\n")
    f.write("#include <cstdint>\n\n")
    f.write("constexpr int16_t NNUE_DATA[] = {\n")
    for i, val in enumerate(data):
        f.write(f"{val}, ")
        if (i + 1) % 16 == 0:
            f.write("\n")
    f.write("};\n")