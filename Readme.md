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

2. Create a build directory and navigate to it:

```sh
mkdir build
cd build
```

3. Configure and build the project:

```sh
cmake ..
cmake --build .
```

The executable will be created in the `build` directory.

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

Special Thanks to [tissatussa](https://github.com/tissatussa) For testing, support and suggestions.

Stay tuned for updates!
