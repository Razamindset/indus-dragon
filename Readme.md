<p align="center">
  <img src="indus-dragon-icon.webp" alt="Indus Dragon" width="150" style="border-radius: 25px;">
</p>
<h1 align="center">Indus Dragon Chess Engine</h1>

Indus Dragon is a UCI chess engine written in C++ that uses the [chess-library](https://github.com/Disservin/chess-library) for board operations and move validation.

[Join our Discord Channel](https://discord.gg/ZBW5DBw8)

[Play the engine](https://lichess.org/@/indusdragon)

## Building the Project

### Prerequisites

- CMake (version 3.15 or higher)
- A C++ compiler that supports C++17
- Git (optional, only needed if you want to clone the repository)

### Build Instructions

1. Clone the repository (if you haven't already):

```sh
git clone https://github.com/Razamindset/indus-dragon
cd indus-dragon
```

2. Build the executable

```sh
make
```

The executable will be created in the root of the project.
Make sure the .nnue file is in the same directory form where u run the programme.
U can also use cmake if u want. Pretty same Configration just make sure to add paths for all the files.

3. Clean .o files

```sh
make clean
```

## Running the Engine

After building, you can run the engine from the build directory:

```sh
./indus-dragon
```

## Contributing

See CONTRIBUTING.md

**⚠️ Important Notice ⚠️**

The engine is constantly changing and improving. It is very possible that a single commit improves or breaks something. As small parameters to different stratigies can change the overall output. We expect u to share your suggestions and findings with us.

---

## Future Plans

- Beat stockfish

## Attribution Notices

Indus Dragon uses:

- [chess-library](https://github.com/Disservin/chess-library) for board representation
- Techniques and algorithms from [Chess Programming Wiki](https://www.chessprogramming.org)
- NNUE probing Library from [dshawul](https://github.com/dshawul) [Library](https://github.com/dshawul/nnue-probe)

Special Thanks to [tissatussa](https://github.com/tissatussa) For testing, support and suggestions.

Stay tuned for updates!
