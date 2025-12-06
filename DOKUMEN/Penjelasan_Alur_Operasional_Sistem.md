# PENJELASAN ALUR OPERASIONAL SISTEM KONTROL MOTOR DUAL BOARD

## RINGKASAN SISTEM

Sistem ini merupakan implementasi kontrol motor ganda yang terdiri dari dua ESP (Plant Board ESP32 dan Interface Board ESP8266) yang berkomunikasi secara wireless menggunakan protokol ESP-NOW untuk mengontrol dua jenis motor: DC Motor untuk eskalator dan AC Motor untuk konveyor. Sistem ini mengintegrasikan kontrol PID real-time, monitoring visual melalui TFT display, dan penyimpanan parameter persisten di EEPROM untuk menciptakan solusi kontrol motor yang robust dan user-friendly.

---

## 1. ALUR OPERASIONAL PLANT BOARD (ESP32)

### 1.1 Inisialisasi Sistem Plant Board

Saat Plant Board dihidupkan, sistem memulai serangkaian proses inisialisasi yang terstruktur. Pertama, brownout detector dinonaktifkan untuk mencegah restart tidak terduga saat beban tinggi. Kemudian sistem menginisialisasi kontrol LED untuk indikator status, diikuti dengan setup komunikasi ESP-NOW untuk koneksi wireless dengan Interface Board. 

Setelah komunikasi siap, sistem mengakses EEPROM untuk memuat parameter PID yang telah disimpan sebelumnya. Jika ini adalah boot pertama kali (magic number tidak ditemukan di EEPROM), sistem akan menggunakan nilai default: untuk DC Motor (Kp=1.5, Ki=0.1, Kd=0.05) dan untuk AC Motor (Kp=2.0, Ki=0.08, Kd=0.04). Parameter-parameter ini kemudian disimpan ke EEPROM untuk penggunaan berikutnya.

Proses inisialisasi berlanjut dengan setup encoder untuk kedua motor. Untuk DC Motor, encoder dikonfigurasi dengan interrupt pada pin A (CHANGE edge) untuk deteksi quadrature, dan timer 50ms diaktifkan. Untuk AC Motor, prosesnya serupa namun dengan interval timer yang dapat disesuaikan. Kalman Filter juga diinisialisasi untuk AC Motor dengan parameter process_noise=0.01 dan measure_noise=3.0 untuk mengurangi noise pada pembacaan RPM.

Sebagai langkah terakhir inisialisasi, Plant Board mengirimkan semua parameter PID yang telah dimuat ke Interface Board melalui ESP-NOW, memastikan kedua board memiliki state awal yang sinkron. Setelah setup selesai, sistem memasuki main loop untuk operasi kontinu.

### 1.2 Main Loop Plant Board

Main loop Plant Board beroperasi dengan prinsip event-driven yang efisien. Loop utama terus memeriksa dua flag boolean: `dcspeedRequest` dan `acspeedRequest` yang menentukan motor mana yang aktif. Flag-flag ini diset oleh Interface Board melalui perintah ESP-NOW (MSG_SPD_REQUEST) ketika user memasuki mode Graph View pada menu.

Ketika `dcspeedRequest` bernilai TRUE, sistem mencetak data encoder DC ke serial monitor untuk debugging, kemudian memanggil fungsi `DC_ProsesPID()` untuk memproses kontrol motor DC. Proses serupa terjadi untuk AC Motor ketika `acspeedRequest` aktif, dengan pemanggilan fungsi `AC_ProsesPID()`. Loop ini berjalan terus-menerus tanpa delay blocking, memastikan respons real-time terhadap perubahan kondisi dan perintah dari Interface Board.

### 1.3 Kontrol DC Motor dengan PID

Kontrol DC Motor dijalankan melalui mekanisme timer interrupt yang dipicu setiap 50ms (DCREAD_INTERVAL). Ketika interrupt timer `DConTimer()` terpicu, sistem pertama mengecek flag `dcspeedRequest`. Jika FALSE, motor akan dimatikan dengan memanggil `DCmotorControl(0, 0)` dan flag `DCnewDataReady` di-set FALSE. Jika TRUE, flag `DCnewDataReady` di-set TRUE untuk memberi sinyal bahwa data baru siap diproses.

Di main loop, ketika flag `DCnewDataReady` terdeteksi TRUE, sistem memasuki critical section menggunakan `portENTER_CRITICAL` untuk melindungi variabel shared dari race condition. Di dalam critical section, sistem menghitung pulse count dengan mengurangi posisi encoder saat ini dengan posisi sebelumnya (`DCpulseCount = DCencoder - DClastEncoder`), kemudian menghitung RPM menggunakan formula: `DCrpm = (abs(DCpulseCount) * 60000) / (DCREAD_INTERVAL * PPR)`. Karena motor DC menggunakan gearbox dengan ratio 200:1, RPM final dihitung sebagai `DCGearboxRPM = DCrpm / 200`. Critical section kemudian ditutup dengan `portEXIT_CRITICAL`.

Setelah RPM dihitung, sistem mengecek mode operasi motor. Jika `DCMode` TRUE dan `PIDMODE` TRUE, kontrol PID dijalankan. Pertama, error dihitung sebagai selisih antara setpoint dan RPM aktual (`Error = DCsetpoint - DCGearboxRPM`). Kemudian tiga komponen PID dihitung: Proportional (`P = DCkP × Error`), Integral dengan anti-windup (`DCintegralSum += Error × dt`, dengan clamping ke DC_INTEGRAL_MAX/MIN), dan Derivative (`D = DCkD × (Error - DCpreviousError) / dt`). Output PID dihitung sebagai `output = (P + I + D) × 10`.

Output kemudian melalui saturation checking untuk mencegah overflow. Jika output > 4095, di-clamp ke 4095 (maksimal PWM 12-bit). Jika output < -4095, di-clamp ke -4095. Sistem juga mengimplementasikan anti-stall protection: jika 0 < output < 800 atau -800 < output < 0, output dinaikkan ke ±800 untuk memastikan motor mendapat torsi minimal untuk berputar.

Jika mode adalah manual (`DCMode` TRUE tapi `PIDMODE` FALSE), output PWM langsung di-map dari setpoint (0-115) ke range PWM (0-4095) tanpa perhitungan PID. Jika motor dimatikan (`DCMode` FALSE), `DCmotorControl(0, 0)` dipanggil untuk mematikan motor.

Setelah kontrol selesai, sistem mengirim feedback ke Interface Board melalui ESP-NOW: nilai `DCGearboxRPM` (MSG_DC_SPEED) dan timestamp (`millis() - waktu_awal`) untuk sinkronisasi grafik. Data juga dicetak ke serial monitor untuk monitoring dan debugging.

### 1.4 Kontrol AC Motor dengan PID dan Kalman Filter

Kontrol AC Motor memiliki kompleksitas tambahan dengan implementasi Kalman Filter untuk mengatasi noise pada pembacaan encoder. Timer interrupt `AConTimer()` bekerja dengan interval yang dapat diatur (biasanya lebih lambat dari DC untuk mengurangi beban prosesor), men-set flag `ACnewDataReady` saat `acspeedRequest` aktif.

Ketika `ACnewDataReady` TRUE di main loop, sistem memanggil fungsi `ACgetRPM()` yang mengintegrasikan Kalman Filter. Proses Kalman Filter terdiri dari empat langkah: (1) Prediction step - estimasi saat ini tetap, error covariance ditambah process noise (`AC_error_estimate += AC_process_noise`), (2) Kalman Gain calculation - menghitung bobot optimal antara estimasi dan measurement (`AC_kalman_gain = AC_error_estimate / (AC_error_estimate + AC_error_measure)`), (3) Update step - meng-update estimasi berdasarkan measurement baru (`AC_estimate += AC_kalman_gain × (measurement - AC_estimate)`), dan (4) Error covariance update (`AC_error_estimate = (1 - AC_kalman_gain) × AC_error_estimate`). Hasil akhirnya adalah `ACrpm` yang sudah difilter.

Setelah RPM filtered diperoleh, sistem memasuki critical section ISR (`portENTER_CRITICAL_ISR`) dan memanggil fungsi `AC_PID()` dengan parameter setpoint, RPM saat ini, dan delta time. Perhitungan PID dilakukan serupa dengan DC Motor: Error, Proportional, Integral dengan anti-windup (clamp ke AC_INTEGRAL_MAX/MIN), dan Derivative. Perbedaannya, output AC Motor dihitung sebagai `output = ACProportional + ACIntegral + ACDerivative` (tanpa pengali 10).

Saturation checking untuk AC Motor membatasi output ke range 0-255 (8-bit PWM) dengan anti-stall protection di threshold 800. Jika motor dimatikan (`ACMode` FALSE), tidak hanya motor dimatikan dengan `ACmotorControl(false, 0, true, 0)`, tetapi juga integral sum dan previous error direset ke 0 untuk mencegah integral windup saat motor dihidupkan kembali.

### 1.5 Komunikasi ESP-NOW di Plant Board

Plant Board berfungsi sebagai receiver untuk perintah kontrol dan transmitter untuk data feedback. Fungsi callback `OnDataReceived()` dipanggil otomatis ketika paket ESP-NOW diterima dari Interface Board. Setiap paket berukuran 8 bytes: 4 bytes untuk typeId (identifier jenis pesan) dan 4 bytes untuk value (data float).

Sistem menggunakan switch-case untuk mem-parsing typeId. Untuk parameter PID (MSG_DC_KP, MSG_DC_KI, MSG_DC_KD, MSG_AC_KP, MSG_AC_KI, MSG_AC_KD dengan typeId 11-13 dan 21-23), nilai parameter diupdate dan segera disimpan ke EEPROM menggunakan `EEPROM.put()` diikuti `EEPROM.commit()` untuk persistent storage. Untuk setpoint (MSG_DC_Setpoint typeId 14, MSG_AC_Setpoint typeId 24), nilai hanya disimpan di RAM karena bersifat runtime dan tidak perlu persisten.

Perintah kontrol motor (MSG_DC_Control typeId 15, MSG_AC_Control typeId 25) mengatur flag `DCMode` dan `ACMode` untuk menghidupkan/mematikan motor. MSG_PID_MODE (typeId 30) mengatur mode PID atau Manual untuk kedua motor. MSG_SPD_REQUEST (typeId 31) sangat penting karena menentukan motor mana yang aktif: value 1 untuk DC saja, 2 untuk AC saja, 3 untuk keduanya. Ketika speed request aktif, `waktu_awal_motor` dicatat dengan `millis()` untuk timestamp referensi.

Pesan khusus ESP_RESTART (typeId 100) memicu restart Plant Board melalui `ESP.restart()`, berguna untuk recovery dari error state atau update konfigurasi.

Untuk pengiriman data, fungsi `sendTaggedFloat()` membuat paket 8-byte dengan struktur yang sama dan mengirimkannya via `esp_now_send()` ke MAC address Interface Board. Callback `OnDataSent()` memberikan konfirmasi status pengiriman (success atau fail) untuk monitoring reliabilitas komunikasi.

---

## 2. ALUR OPERASIONAL INTERFACE BOARD (ESP8266)

### 2.1 Inisialisasi Sistem Interface Board

Interface Board memulai dengan inisialisasi serial communication pada baudrate 115200 untuk debugging. Kemudian ESP-NOW diinisialisasi untuk komunikasi dengan Plant Board, diikuti setup rotary encoder untuk input navigasi menu. Pin encoder A dan B dikonfigurasi dengan pull-up internal, dan interrupt diaktifkan pada encoder A untuk deteksi rotasi.

TFT Display ST7735 diinisialisasi dengan `tft.initR(INITR_BLACKTAB)` dan rotasi diatur ke mode 0 (portrait). Menu system kemudian diinisialisasi dengan `menuInit(&tft)` yang menampilkan splash screen selama 2 detik untuk memberikan feedback visual bahwa sistem siap. Sebagai langkah final, Interface Board mengirim perintah `ESP_RESTART` (typeId 100) ke Plant Board untuk memastikan kedua board memulai dalam state yang bersih dan sinkron.

### 2.2 Main Loop Interface Board

Main loop Interface Board mengimplementasikan polling-based input processing dengan update display yang efisien. Loop dimulai dengan menyimpan posisi encoder sebelumnya (`lastEncoder = currentEncoder`), kemudian membaca posisi saat ini dengan `DCgetEncoderCount()`. Jika posisi berubah, sistem menghitung direction (1 untuk CW, -1 untuk CCW) dan memanggil `menuNavigate(direction)` untuk menggerakkan cursor di menu atau mengubah nilai parameter jika sedang dalam mode editing.

Setelah encoder processing, sistem membaca button dengan `getButtonClick()` yang mendeteksi tiga jenis click: Single Click (press < 500ms) untuk select/confirm, Double Click (2x press dalam < 300ms) untuk toggle motor run/stop atau encoder mode, dan Long Press (hold > 1000ms) untuk back/cancel. Setiap jenis click memicu action yang berbeda sesuai context menu saat ini.

Fungsi `menuUpdate()` dipanggil di setiap iterasi loop untuk update display. Fungsi ini mengecek flag `g_escSpeedUpdated` dan `g_motorSpeedUpdated` yang di-set TRUE oleh callback ESP-NOW ketika data baru tiba. Jika flag TRUE, graph di-update dengan incremental redraw (oscilloscope style) untuk efisiensi, kemudian flag di-reset ke FALSE. Loop ditutup dengan `delay(10ms)` untuk mencegah excessive polling yang membuang-buang CPU cycles.

### 2.3 Menu Navigation System

Menu system diorganisir dalam struktur hierarki tiga level dengan state machine. Level 0 (Root Menu) berisi tiga pilihan: [0] Escalator, [1] Motor AC, dan [2] WiFi Status. Ketika user meng-select Escalator atau Motor AC, menu berpindah ke Level 1 yang berisi dua submenu: [0] Graph View untuk monitoring real-time dan [1] Control View untuk setting parameter.

Dalam Graph View, display menampilkan grafik real-time bergaya oscilloscope dengan update incremental setiap kali data baru datang dari Plant Board. Grafik menampilkan RPM motor (sumbu Y) versus waktu (sumbu X) dengan garis setpoint horizontal berwarna merah sebagai referensi target. User dapat toggle motor ON/OFF dengan double-click tanpa perlu masuk ke Control View, memberikan kontrol cepat saat monitoring.

Control View menampilkan parameter PID dan kontrol motor dalam format list. Untuk DC Motor: [0] Kp, [1] Ki, [2] Kd, [3] Setpoint (0-115 RPM), [4] Run/Stop, dan [5] Direction (Forward/Reverse). Untuk AC Motor serupa tetapi Setpoint range-nya 0-1500 RPM. User menavigasi dengan memutar encoder, single-click untuk masuk mode editing, putar encoder untuk mengubah nilai (dengan auto-increment/decrement sesuai encoder direction mode), dan single-click lagi untuk confirm dan mengirim nilai baru ke Plant Board via ESP-NOW.

Navigasi back dilakukan dengan long-press encoder button. Jika sedang editing, long-press akan cancel tanpa save perubahan. Jika di submenu (Graph atau Control View), long-press kembali ke menu parent (Escalator/Motor AC). Jika di Level 1, long-press kembali ke Root Menu. State machine memastikan transisi yang smooth dan konsisten di seluruh menu system.

### 2.4 Real-time Graph Display

Graph display mengimplementasikan rendering oscilloscope-style yang efisien dan intuitif. Area graph berukuran 118×70 pixels di TFT display, dengan sumbu X merepresentasikan waktu (scrolling dari kanan ke kiri) dan sumbu Y merepresentasikan RPM motor. Sistem menggunakan circular buffer dengan kapasitas 100 samples untuk menyimpan history data.

Ketika data speed baru diterima dari Plant Board via ESP-NOW callback `receiveData()`, nilai RPM disimpan ke circular buffer `g_escSpeedHistory[]` atau `g_motorSpeedHistory[]` dengan index yang increment secara modulo 100 (`g_escHistoryIndex = (g_escHistoryIndex + 1) % 100`). Flag `g_escSpeedUpdated` atau `g_motorSpeedUpdated` kemudian di-set TRUE untuk memberi sinyal ke main loop bahwa graph perlu update.

Di main loop, fungsi `menuUpdate()` mendeteksi flag TRUE dan memanggil `drawEscGraph()` atau `drawMotorGraph()`. Ada dua mode rendering: Full Redraw dan Incremental Update. Full Redraw dilakukan pada initial display atau ketika user berpindah menu, membersihkan area graph, menggambar border putih, menggambar garis setpoint horizontal (merah, dashed), menggambar grid waktu (vertikal cyan setiap 1 detik), dan plotting semua 100 samples dari buffer.

Incremental Update (mode normal) jauh lebih efisien: sistem menggeser semua pixel grafik 1 pixel ke kiri dengan loop yang erase pixel lama (hitam) dan redraw pixel dari kolom sebelah kanan (kuning), kemudian menggambar data terbaru di kolom paling kanan. Teknik ini menciptakan efek scrolling smooth seperti oscilloscope tanpa perlu full screen redraw, menghemat waktu render dan mengurangi flicker.

Koordinat Y dihitung dengan formula: `y = 94 - (value × 69 / MAX_VALUE)` dimana 94 adalah posisi bottom graph, 69 adalah tinggi graph area, dan MAX_VALUE adalah 115 untuk DC atau 1500 untuk AC. Hasil kemudian di-clamp ke range [25, 94] untuk memastikan pixel tetap dalam graph area. Array `escGraphY[]` dan `motorGraphY[]` menyimpan posisi Y setiap kolom untuk optimasi redraw.

### 2.5 Encoder Input Processing

Rotary encoder processing menggabungkan interrupt-driven dan polling-based approach. Encoder menggunakan quadrature encoding dengan dua signal: Channel A dan Channel B yang memiliki phase offset 90 derajat. Interrupt di-attach ke Channel A pada CHANGE edge, sehingga interrupt terpicu pada rising dan falling edge.

Di interrupt handler `DChandleEncoderA()` yang di-declare dengan attribute `IRAM_ATTR` untuk eksekusi cepat dari RAM, sistem memasuki critical section dengan `portENTER_CRITICAL_ISR(&timerMux)`, membaca state Channel A dan B, kemudian melakukan quadrature decoding: jika A rising dan B LOW atau A falling dan B HIGH, encoder bergerak forward (increment counter); sebaliknya jika A rising dan B HIGH atau A falling dan B LOW, encoder bergerak backward (decrement counter). Critical section ditutup dengan `portEXIT_CRITICAL_ISR(&timerMux)` untuk release mutex.

Di main loop, sistem polling encoder count dengan membandingkan `currentEncoder` dengan `lastEncoder`. Jika berbeda, direction dihitung (1 atau -1), dan action dilakukan tergantung mode: jika `g_isEditing` TRUE (sedang edit parameter), nilai parameter diubah sesuai direction dan `encoderMode` (ENCODER_UP untuk increment, ENCODER_DOWN untuk decrement); jika `g_isEditing` FALSE, `menuNavigate(direction)` dipanggil untuk scroll menu.

Button encoder diproses dengan debouncing dan multi-click detection. State button dibaca dengan `digitalRead()`, kemudian algoritma timing-based mendeteksi: Single Click (press duration < 500ms, release, tidak ada second click dalam 300ms), Double Click (press-release-press sequence dalam < 300ms total), dan Long Press (press duration > 1000ms). Setiap jenis click men-trigger action berbeda sesuai context menu yang sesuai.

Fitur `toggleEncoderMode()` dipanggil saat double-click di non-graph context, membalik arah increment/decrement encoder. Ini berguna saat editing parameter: user bisa cepat switch antara "putar kanan = naik" dan "putar kanan = turun" sesuai preferensi, meningkatkan ergonomics terutama saat fine-tuning parameter PID.

### 2.6 Komunikasi ESP-NOW di Interface Board

Interface Board berfungsi sebagai master controller yang mengirim perintah dan menerima feedback data. Callback `receiveData()` mem-parsing paket 8-byte ESP-NOW sama seperti di Plant Board. Untuk message type MSG_DC_SPEED (typeId 10) dan MSG_AC_SPEED (typeId 20), nilai RPM di-extract dan fungsi `updateEscalatorSpeed()` atau `updateMotorSpeed()` dipanggil.

Fungsi update speed melakukan tiga hal: (1) menyimpan nilai ke circular buffer history untuk graphing, (2) meng-update variabel `g_escSpeed` atau `g_motorSpeed` untuk display numerik, dan (3) men-set flag `g_escSpeedUpdated` atau `g_motorSpeedUpdated` TRUE untuk trigger graph redraw di main loop. Flag `g_escConnected` dan `g_motorConnected` juga di-set TRUE untuk indikator status koneksi ESP-NOW.

Message type parameter PID (MSG_DC_KP hingga MSG_DC_KD dan MSG_AC_KP hingga MSG_AC_KD) diterima sebagai echo confirmation dari Plant Board. Nilai parameter di-update di Interface Board untuk menjaga sinkronisasi display dengan actual value di Plant Board. MSG_TIMESTAMP (typeId 32) diterima dan disimpan ke timestamp buffer untuk sinkronisasi waktu pada graph axis.

Untuk pengiriman perintah, fungsi `sendTaggedFloat()` dipanggil dari berbagai tempat di menu system. Saat user mengubah parameter PID, fungsi ini dipanggil dengan typeId yang sesuai (contoh: `sendTaggedFloat(MSG_DC_KP, g_dcKp)`). Saat toggle motor ON/OFF, `sendTaggedFloat(MSG_DC_Control, g_escRunning ? 1.0 : 0.0)` dikirim. Saat masuk Graph View, `sendTaggedFloat(MSG_SPD_REQUEST, requestValue)` dikirim untuk mengaktifkan data streaming dari Plant Board.

Callback `onDataSent()` memberikan feedback status pengiriman. Jika status adalah `ESP_NOW_SEND_SUCCESS`, log dicetak ke serial. Jika fail, error dicatat dan flag `g_espnowInit` bisa di-reset FALSE untuk trigger reconnection attempt. Reliable delivery ESP-NOW memastikan command critical tidak hilang dalam transmisi wireless.

---

## 3. INTERAKSI SISTEM SECARA KESELURUHAN

### 3.1 Skenario Operasional: User Mengubah Setpoint DC Motor

Proses dimulai ketika user menavigasi ke menu Escalator > Control View menggunakan rotary encoder. User scroll dengan memutar encoder hingga highlight berada di "Setpoint", kemudian single-click untuk masuk mode editing. Indikator visual (misalnya warna atau border berkedip) muncul di display menunjukkan parameter sedang di-edit.

User memutar encoder untuk mengubah nilai setpoint dari, misalnya, 50 RPM ke 80 RPM. Setiap detent rotasi encoder mengubah nilai dengan increment tertentu (misalnya ±1 atau ±5 tergantung range). Display real-time menampilkan nilai baru saat encoder diputar. User kemudian single-click lagi untuk confirm perubahan.

Saat confirm, Interface Board memanggil `sendTaggedFloat(MSG_DC_Setpoint, 80.0)` yang membuat paket 8-byte: `[0x00, 0x00, 0x00, 0x0E]` (typeId 14 untuk MSG_DC_Setpoint) + `[0x42, 0xA0, 0x00, 0x00]` (float 80.0 dalam IEEE 754). Paket dikirim via ESP-NOW dengan latency typical < 10ms.

Di Plant Board, callback `OnDataReceived()` terpicu, mem-parsing typeId 14, meng-extract value 80.0, dan meng-update variabel `DCsetpoint = 80.0`. Nilai ini langsung aktif untuk PID calculation di interrupt timer berikutnya. Pada interrupt cycle berikutnya (50ms), PID controller menghitung error baru (`Error = 80.0 - DCGearboxRPM`), melakukan kalkulasi P, I, D, dan menghasilkan output PWM yang lebih tinggi untuk mempercepat motor mencapai setpoint baru.

Saat motor mempercepat, encoder Plant Board mendeteksi kenaikan RPM. Data RPM baru dikirim kembali ke Interface Board via `sendTaggedFloat(MSG_DC_SPEED, DCGearboxRPM)` setiap 50ms. Interface Board menerima update speed, menyimpannya ke buffer history, dan graph di-update secara incremental, menampilkan kurva RPM yang naik menuju garis setpoint baru (80 RPM). User dapat melihat response real-time system terhadap perubahan setpoint, termasuk overshoot, settling time, dan steady-state error yang menunjukkan kualitas tuning PID.

### 3.2 Skenario Operasional: User Melakukan Tuning PID

User ingin melakukan fine-tuning parameter PID DC Motor untuk mendapatkan response optimal. Prosesnya dimulai dengan navigasi ke Escalator > Control View, kemudian scroll ke parameter Kp. User single-click untuk edit, ubah nilai dari 1.5 ke 2.0 dengan memutar encoder, kemudian single-click untuk confirm. Interface Board mengirim `sendTaggedFloat(MSG_DC_KP, 2.0)` ke Plant Board.

Plant Board menerima pesan, meng-update `DCkP = 2.0`, dan yang krusial, menyimpan nilai baru ke EEPROM dengan sequence: `EEPROM.put(ADDRESS_DC_KP, DCkP)` diikuti `EEPROM.commit()`. Proses write ke EEPROM memakan waktu beberapa milliseconds, tetapi dilakukan secara non-blocking sehingga tidak mengganggu timer interrupt dan kontrol motor. Serial monitor mencetak konfirmasi: "DC Kp updated to 2.0 and saved to EEPROM".

User kemudian melakukan hal serupa untuk Ki dan Kd. Setelah semua parameter baru tersimpan, user berpindah ke Graph View untuk observe behavior motor dengan parameter baru. Double-click pada encoder untuk toggle motor ON. Motor start dan Interface Board mengirim `sendTaggedFloat(MSG_DC_Control, 1.0)` serta `sendTaggedFloat(MSG_SPD_REQUEST, 1.0)` untuk activate DC motor dan request speed data streaming.

Plant Board menerima control command, men-set `DCMode = TRUE`, dan motor mulai berputar dengan PID control menggunakan parameter baru. Data speed streaming dimulai, dan graph di Interface Board menampilkan response transient: user dapat observe rise time, overshoot percentage, settling time, dan steady-state behavior. Jika response tidak memuaskan (misalnya overshoot terlalu besar), user bisa kembali ke Control View, adjust Kp lebih rendah atau tingkatkan Kd, dan repeat prosesnya hingga mendapat tuning optimal.

Ketika Plant Board di-restart (power cycle atau perintah ESP_RESTART), setup routine membaca EEPROM, menemukan magic number valid (0x12345678), dan load semua parameter PID yang telah disimpan, termasuk Kp, Ki, Kd yang baru. Ini memastikan tuning user tidak hilang dan motor langsung beroperasi dengan parameter optimal tanpa perlu re-tuning setiap restart.

### 3.3 Skenario Operasional: Monitoring Dual Motor Simultaneous

User ingin memonitor kedua motor secara bersamaan untuk observe interaksi atau compare performance. Dari Root Menu, user select Escalator dan otomatis masuk Graph View untuk DC Motor. Graph menampilkan RPM DC Motor real-time. User kemudian long-press untuk back ke Root Menu, select Motor AC, dan masuk Graph View AC Motor.

Untuk monitoring kedua motor sekaligus, user perlu mengatur agar Interface Board request data dari kedua motor. Ini dilakukan dengan mengirim `sendTaggedFloat(MSG_SPD_REQUEST, 3.0)` dimana value 3 berarti "request both DC and AC". Plant Board mem-parsing value 3, men-set `dcspeedRequest = TRUE` dan `acspeedRequest = TRUE`, sehingga kedua motor mulai streaming data.

Di Plant Board, timer interrupt DC (50ms) dan AC (varies) berjalan independent. Setiap timer terpicu, kalkulasi PID dilakukan, motor dikontrol, dan data RPM dikirim via ESP-NOW dengan typeId berbeda: MSG_DC_SPEED (10) untuk DC dan MSG_AC_SPEED (20) untuk AC. Paket-paket ini dikirim asynchronous tanpa blocking satu sama lain.

Interface Board menerima kedua stream data via callback yang sama `receiveData()`. Callback mem-parsing typeId untuk menentukan data mana yang update: jika typeId 10, update DC buffer dan set flag `g_escSpeedUpdated`; jika typeId 20, update AC buffer dan set flag `g_motorSpeedUpdated`. Main loop kemudian update kedua graph sesuai context menu yang ditampilkan.

User bisa toggle antar Graph View DC dan AC dengan navigasi menu untuk compare secara visual bagaimana kedua motor respond terhadap setpoint changes, load disturbances, atau parameter tuning. Timestamp yang dikirim via MSG_TIMESTAMP memastikan data dari kedua motor ter-sinkronisasi secara temporal, memungkinkan analisis timing-critical jika ada interdependensi antara eskalator (DC) dan konveyor (AC) dalam aplikasi real-world.

### 3.4 Failure Scenarios dan Recovery Mechanisms

Sistem didesain robust dengan beberapa failure recovery mechanisms. Jika koneksi ESP-NOW terputus (misalnya karena interference atau jarak terlalu jauh), Plant Board mendeteksi tidak ada speed request dalam periode tertentu dan automatic mematikan motor sebagai safety feature. Interface Board mendeteksi tidak ada data incoming, men-reset flag `g_escConnected` dan `g_motorConnected` ke FALSE, dan menampilkan indikator "Connection Lost" di display.

User dapat memicu reconnection dengan long-press encoder untuk back ke Root Menu kemudian re-enter Graph View, atau dengan mengirim ESP_RESTART command yang me-restart Plant Board dan re-inisialisasi ESP-NOW. Setelah reconnect, kedua board exchange MAC address lagi dan komunikasi resume normal.

Jika EEPROM corrupt (misalnya power loss during write), Plant Board mendeteksi magic number invalid saat boot, load default parameters, dan menulis magic number baru ke EEPROM beserta default values. Serial monitor mencetak warning "EEPROM corrupted, loading defaults", memberi informasi ke developer atau technician bahwa re-tuning mungkin diperlukan.

Jika motor stall atau load berlebih, anti-stall protection mechanism di PID controller memastikan PWM minimal 800 selalu applied, mencegah motor stuck di torsi rendah. Jika stall persist, user dapat observe di graph bahwa RPM tidak naik meskipun PWM high, indicating mechanical problem atau load terlalu berat. User kemudian bisa lower setpoint atau investigate mechanical issue.

Brownout protection dinonaktifkan di Plant Board untuk prevent false restart saat motor draw high current pada startup. Namun jika voltage drop terlalu severe (below brownout threshold despite disable attempt), ESP akan inevitably reset, tetapi setelah boot akan automatic load PID parameters dari EEPROM dan resume operation with minimal disruption.

---

## 4. KEUNGGULAN ARSITEKTUR SISTEM

### 4.1 Separation of Concerns

Sistem mengimplementasikan clean separation antara control logic (Plant Board) dan user interface (Interface Board). Plant Board fokus pada real-time control: PID calculation, motor driving, encoder reading, dan sensor fusion (Kalman Filter). Interface Board fokus pada human-machine interaction: menu navigation, display rendering, input processing, dan data visualization. Pemisahan ini memungkinkan optimasi independent: Plant Board bisa run PID loop di fixed-rate timer interrupt tanpa terganggu UI rendering, sementara Interface Board bisa implement complex menu logic dan graph animation tanpa jitter timing-critical control loop.

### 4.2 Real-time Performance dengan Reliability

Penggunaan timer interrupt dan ISR (Interrupt Service Routine) memastikan PID control berjalan deterministic dengan timing presisi. DC Motor di-control setiap exact 50ms, AC Motor sesuai configured interval. Critical section protection (`portENTER_CRITICAL`) mencegah race condition pada shared variable antara ISR dan main task. Kalman Filter mengurangi measurement noise, meningkatkan stability control loop terutama untuk AC Motor yang lebih susceptible terhadap electrical noise.

ESP-NOW dipilih untuk komunikasi karena low latency (< 10ms typical), tanpa overhead TCP/IP stack, dan reliable delivery dengan ACK mechanism. Bandwidth cukup untuk streaming 20 Hz (50ms interval) data speed plus occasional parameter updates tanpa congestion.

### 4.3 User Experience dan Usability

Interface Board menyediakan intuitive graphical interface yang menampilkan real-time system behavior. Oscilloscope-style graph dengan scrolling animation memberikan feedback visual yang powerful untuk understand system dynamics. User bisa langsung see cause-effect relationship: adjust Kp → observe overshoot increase, adjust Kd → see damping improve.

Multi-click detection pada encoder button (single/double/long) memaksimalkan single input device untuk multiple functions, reducing hardware complexity. Hierarchical menu structure dengan logical grouping (motor selection → view selection → parameter selection) matches user mental model dan minimizes navigation steps.

### 4.4 Persistent Configuration dan Maintenance-Free

PID parameters disimpan di EEPROM dengan magic number validation memastikan configuration persist across power cycles. Technician bisa tune system once di commissioning phase, dan tuning tersebut retained forever. Jika parameter needs adjustment during maintenance, changes bisa dilakukan via GUI (Interface Board) dan automatic saved tanpa perlu USB cable atau programming tools.

Dual-board architecture juga memudahkan maintenance: jika Interface Board rusak (display crack, encoder wear out), Plant Board tetap bisa operate standalone dengan last known parameters. Sebaliknya, jika Plant Board perlu replacement, Interface Board bisa send parameters via ESP-NOW untuk configure replacement board automatically.

### 4.5 Scalability dan Extensibility

Arsitektur message-passing dengan typed protocol (typeId system) memudahkan extension. Untuk menambah motor ketiga, cukup define new message types (MSG_MOTOR3_KP, MSG_MOTOR3_SPEED, etc.) dan implement handler di callback. Menu system bisa di-extend dengan menambah menu item baru tanpa refactor existing code.

Circular buffer dan incremental rendering untuk graph bisa scale ke multiple concurrent graphs dengan memory footprint predictable. Jika future requirement perlu display kedua motor di split-screen, architecture sudah support karena update mechanism per-motor sudah independent dengan flag system (`g_escSpeedUpdated` vs `g_motorSpeedUpdated`).

---

## KESIMPULAN

Sistem kontrol motor dual-board ini merepresentasikan implementasi industrial-grade embedded system dengan fokus pada real-time performance, reliability, dan usability. Plant Board ESP32 menjalankan deterministic control loop dengan PID algorithm yang dilengkapi anti-windup, anti-stall protection, dan Kalman filtering untuk robust operation. Interface Board ESP8266 menyediakan rich user interface dengan graphical feedback, intuitive navigation, dan comprehensive parameter configuration capabilities.

Komunikasi wireless ESP-NOW memungkinkan kedua board beroperasi dengan loose coupling, meningkatkan reliability (isolated failure) dan flexibility (physical separation). Persistent storage di EEPROM memastikan configuration stability across operational lifecycle. Architecture yang modular dan message-oriented memudahkan maintenance, debugging, dan future enhancement.

Sistem ini cocok untuk aplikasi industrial automation seperti conveyor systems, escalators, material handling, atau automated production lines dimana dual motor coordination, precise speed control, real-time monitoring, dan ease of parameter tuning menjadi critical requirements.

