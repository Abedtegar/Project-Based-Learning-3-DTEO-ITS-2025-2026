# Encoder Test Program

## Cara Menggunakan:

### 1. Backup main.cpp yang asli
```bash
# Rename file main.cpp yang ada
mv src/src/main.cpp src/src/main_backup.cpp
```

### 2. Copy encoder_test.cpp ke main.cpp
```bash
cp test/encoder_test.cpp src/src/main.cpp
```

### 3. Upload ke board
```bash
platformio run --target upload
```

### 4. Monitor Serial Output
```bash
platformio device monitor
```

### 5. Test Encoder
- Putar encoder perlahan searah jarum jam -> harus muncul "+" dan Position naik
- Putar encoder perlahan berlawanan jarum jam -> harus muncul "-" dan Position turun
- Tekan tombol encoder -> Position reset ke 0

### 6. Yang Harus Diperhatikan:

**Normal Behavior:**
- Setiap detik encoder = 1 perubahan position (±1)
- Symbol +/- muncul konsisten dengan arah putaran
- Trigger rate sekitar 2-4 Hz untuk putaran lambat
- Pin states berubah antara 0 dan 1

**Abnormal Behavior (masalah):**
- Position loncat 2-3 dalam 1 detik
- Symbol +/- muncul bersamaan atau random
- Trigger rate > 20 Hz saat diam
- Pin states selalu sama (0 atau 1)

### 7. Kembalikan ke program normal
```bash
# Restore original main.cpp
mv src/src/main_backup.cpp src/src/main.cpp
platformio run --target upload
```

## Output Example:

```
========================================
ENCODER DEBUG & LOGGING TEST
========================================
CLK Pin: 25
DT Pin: 26
SW Pin: 27
========================================
Rotate encoder to test...
Press button to reset position
+ = Clockwise, - = Counter-clockwise
========================================

Ready! Start rotating encoder...

++
----------------------------------------
Time: 1234 ms
Position: 2 (Change: +2)
Total Triggers: 45
Trigger Rate: 36.5 Hz
Pin States: CLK=1, DT=0, SW=1
----------------------------------------
```
