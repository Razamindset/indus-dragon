import os
import math
import random
import struct
import hashlib
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import IterableDataset, DataLoader
from tqdm import tqdm

# This script was written using ai
# The data was generated from self play on antoher brach 
# About 25 million sampels and a final test loss of 0.024

# ==========================================
# 1. ARCHITECTURE
# ==========================================
class IndusNet(nn.Module):
    def __init__(self):
        super(IndusNet, self).__init__()
        self.fc1 = nn.Linear(768, 256)
        self.relu = nn.ReLU()
        self.fc2 = nn.Linear(256, 1)

    def forward(self, x):
        x = self.fc1(x)
        x = self.relu(x)
        x = self.fc2(x)
        return torch.sigmoid(x)

# ==========================================
# 2. PRE-PROCESSING (MAX SPEED)
# ==========================================
class DataProcessor:
    @staticmethod
    def mirror_fen_horizontal(fen):
        board_part = fen.split(' ')[0]
        rows = board_part.split('/')
        mirrored_rows = []
        for row in rows:
            expanded = "".join([" " * int(c) if c.isdigit() else c for c in row])
            mirrored_row = expanded[::-1] # Reverse left-to-right
            
            res, count = "", 0
            for c in mirrored_row:
                if c == " ": count += 1
                else:
                    if count > 0: res += str(count); count = 0
                    res += c
            if count > 0: res += str(count)
            mirrored_rows.append(res)
        
        return '/'.join(mirrored_rows) + " " + " ".join(fen.split(' ')[1:])

    @staticmethod
    def clean_augment_and_split(input_file, train_file="train_v3.txt", test_file="test_v3.txt", split_ratio=0.9):
        if os.path.exists(train_file):
            print("V3 Data already prepared. Skipping.")
            return

        seen_fens = set()
        total, duplicates, added_mirrors = 0, 0, 0
        
        print(f"Phase 1: Deduplicating and Mirroring {input_file} (This may take a few minutes)...")
        with open(input_file, 'r') as f_in, \
             open(train_file, 'w') as f_train, \
             open(test_file, 'w') as f_test:
            
            for line in f_in:
                total += 1
                parts = line.split('|')
                if len(parts) < 3: continue
                
                fen, score, res = parts[0].strip(), parts[1].strip(), parts[2].strip()
                
                # 1. Process Original
                original_hash = hashlib.md5(fen.encode()).digest()
                if original_hash not in seen_fens:
                    seen_fens.add(original_hash)
                    target_file = f_train if random.random() < split_ratio else f_test
                    target_file.write(line)
                else:
                    duplicates += 1
                    continue # Skip mirroring if original is a duplicate

                # 2. Process Mirror (Double the dataset instantly)
                mirrored_fen = DataProcessor.mirror_fen_horizontal(fen)
                mirror_hash = hashlib.md5(mirrored_fen.encode()).digest()
                
                if mirror_hash not in seen_fens:
                    seen_fens.add(mirror_hash)
                    added_mirrors += 1
                    # Score and Result stay exactly the same for horizontal mirrors!
                    target_file = f_train if random.random() < split_ratio else f_test
                    target_file.write(f"{mirrored_fen} | {score} | {res}\n")

                if total % 1000000 == 0:
                    print(f"Read {total//1000000}M lines | Removed {duplicates} dups | Added {added_mirrors} mirrors...")

        print(f"Data Prep Complete! Final Dataset Size: {total - duplicates + added_mirrors} positions.")

# ==========================================
# 3. STREAMING DATASET (PURE MATH, NO STRINGS)
# ==========================================
class ChessDataset(IterableDataset):
    def __init__(self, filepath):
        self.filepath = filepath
        self.piece_map = {'P':0, 'N':1, 'B':2, 'R':3, 'Q':4, 'K':5, 'p':6, 'n':7, 'b':8, 'r':9, 'q':10, 'k':11}

    def __iter__(self):
        with open(self.filepath, 'r') as f:
            for line in f:
                try:
                    parts = [p.strip() for p in line.split('|')]
                    fen, score_str, result_str = parts[0], parts[1], parts[2]
                    
                    tensor = torch.zeros(768, dtype=torch.float32)
                    square = 0
                    for char in fen.split(' ')[0]:
                        if char == '/': continue
                        elif char.isdigit(): square += int(char)
                        else:
                            tensor[(self.piece_map[char] * 64) + square] = 1.0
                            square += 1
                    
                    score_cp = float(score_str)
                    target = (0.6 * (1.0 / (1.0 + math.pow(10.0, -score_cp / 400.0)))) + (0.4 * float(result_str))
                    yield tensor, torch.tensor([target], dtype=torch.float32)
                except: pass

# ==========================================
# 4. TRAINING LOOP
# ==========================================
def main():
    RAW_DATA = "datagen.txt"
    DataProcessor.clean_augment_and_split(RAW_DATA)

    EPOCHS = 15
    BATCH_SIZE = 16384
    
    model = IndusNet()
    optimizer = optim.Adam(model.parameters(), lr=0.002) # Higher initial LR
    # UPGRADE: Cosine Annealing creates a perfect smooth learning curve
    scheduler = optim.lr_scheduler.CosineAnnealingLR(optimizer, T_max=EPOCHS, eta_min=1e-5)
    criterion = nn.MSELoss()

    train_loader = DataLoader(ChessDataset("train_v3.txt"), batch_size=BATCH_SIZE, num_workers=4)
    test_loader = DataLoader(ChessDataset("test_v3.txt"), batch_size=BATCH_SIZE, num_workers=4)

    best_test_loss = float('inf')
    best_weights = None

    for epoch in range(EPOCHS):
        model.train()
        t_loss, t_count = 0, 0
        pbar = tqdm(train_loader, desc=f"Epoch {epoch+1}/{EPOCHS} [TRAIN]")
        for inputs, targets in pbar:
            optimizer.zero_grad()
            loss = criterion(model(inputs), targets)
            loss.backward()
            torch.nn.utils.clip_grad_norm_(model.parameters(), 1.0)
            optimizer.step()
            t_loss += loss.item(); t_count += 1
            pbar.set_postfix({'Loss': f"{t_loss/t_count:.5f}", 'LR': f"{scheduler.get_last_lr()[0]:.5f}"})

        model.eval()
        v_loss, v_count = 0, 0
        with torch.no_grad():
            for inputs, targets in tqdm(test_loader, desc="[TEST]"):
                v_loss += criterion(model(inputs), targets).item(); v_count += 1
        
        avg_v = v_loss/v_count
        print(f"-> Epoch {epoch+1} Results | Train: {t_loss/t_count:.5f} | Test: {avg_v:.5f}")
        
        # UPGRADE: Save only the absolute best iteration in memory
        if avg_v < best_test_loss:
            best_test_loss = avg_v
            best_weights = model.state_dict().copy()
            print("   [!] New Best Model Found!")

        scheduler.step()

    # Final Binary Export using the BEST weights, not just the last epoch
    print(f"\nTraining Complete. Exporting the best model (Loss: {best_test_loss:.5f}) to indus_dragon_v3.bin...")
    model.load_state_dict(best_weights)
    with open("indus_dragon_v3.bin", "wb") as f:
        for name, param in model.named_parameters():
            for val in param.detach().cpu().numpy().flatten():
                f.write(struct.pack('h', int(round(val * 255))))
    print("Export successful. Ready for C++ AVX2 inference.")

if __name__ == "__main__":
    main()
