# LOG of my work with waterlevel

## 2023-06-06

~1h -> VL53l0x gibt schlechte Werte aus. Problem war, dass Sensor am Stock reflektierte, an dem er befestigt war. FOV ist 25°!!! Länge mal 0,432879228 ergibt Oberflächengröße, die mindestens geben muss. Also wirder angefangen, an Hauptprogramm zu arbeiten.

## 2023-06-10

~3h SNTP versucht zu implementieren. time.h selbst geht, wird aber eben nicht mittels ntp geupdatet. Obwohl esp_sntp.h inkludiert, ging nur das in der Bilbliothek angelegte alias "sntp_...", statt "esp_sntp_...". Hab nun das Beispiel implementiert. Aber weiß nicht, ob sich das automatisch updatet, und dhcp server finden ist auch nicht dabei. Daten kommen nicht durch, connection refused... Die default RTC im esp32 geht auf 20min ca 4sekunden falsch, kann aber auch an meiner Messungenauigkeit liegen.

## 2023-06-11

~3h Funktionen Messen, Zeit updaten und Daten senden separiert. Zeit wird bei jedem Mal geupdatet, über hardcoded Adresse (nicht dhcp), NTP scheint schon in configTime aus time.h dabei zu sein. Benutze dazu also nichts anderes mehr. Nun müssen noch Regeln eingeführt werden, nach denen die Zeit geupdatet wird und die Daten gesendet werden. ESP läuft aber erstmal stabil leider gibt es bei http oft Fehlre so das nur ca 1/5 der Daten durchkommt. Die müssten dann im Falle eines Fehlers noch gespeichert werden.

