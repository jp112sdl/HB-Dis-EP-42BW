# HB-Dis-EP-42BW Platine     [![License: CC BY-NC-SA 3.0](https://img.shields.io/badge/License-CC%20BY--NC--SA%203.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc-sa/3.0/)




# Hardware

### Bauteile

#### Reichelt

[Bestellliste](https://www.reichelt.de/my/)

Bauteil                  | Bestellnummer    | Anzahl | Kommentar
------------------------ | ---------------- | ------ | ---------
C1                       | X5R-G0603 10/6   |   1    | -
C2 .. C5, C7             | X7R-G0603 100N   |   5    | -
C6, C8                   | X7R-G0603 1,0/16 |   2    | -
D1                       | KTB L-115WEGW    |   1    | -
R1                       | RND 0603 1 10K   |   1    | -
R2 .. R3                 | RND 0603 1 330   |   2    | -
R4                       | RND 0603 1 1,0M  |   1    | -
R6                       | RND 0603 1 4,7k  |   1    | optional: für Batteriespannungsmessung unter Last
R7 .. R8                 | RND 0603 1 8,2   |   2    | optional: für Batteriespannungsmessung unter Last
Q1                       | IRLML 6344       |   1    | optional: für Batteriespannungsmessung unter Last
SW0                      | JTP-1130         |   1    | optional: Konfigtaster *Achtung, evtl. andere Bauhöhe!*
SW1 .. SW10              | JTP-1130         |  10    | *Achtung, evtl. andere Bauhöhe!*
U1                       | ATMEGA 1284P-AU  |   1    | -
Y1                       | CSTCE 8,00       |   1    | -
Verbinder zum CC1101     | MPE 156-1-032    |   1    | -
Verbinder zum ePaper     | -    |   1    | -
Verbinder zur Batterie   | -    |   1    | -



#### Farnell

Bauteil                  | Bestellnummer    | Anzahl | Kommentar
------------------------ | ---------------- | ------ | ---------
U3                       | MCP111T-270E/TT  |   1    | optional, Unterspannungsüberwachung


#### Sonstiges

Bauteil | Bestellnummer            | Anzahl | Kommentar
------- | ------------------------ | ------ | ---------
U2      | CC1101 Funkmodul 868 MHz |   1    | z.B. [eBay](https://www.ebay.de/itm/272455136087)

~8,3 cm Draht als Antenne


### Programmieradapter
- 1x ISP (z.B. [diesen hier](https://www.diamex.de/dxshop/USB-ISP-Programmer-fuer-Atmel-AVR-Rev2))


# Software

### Fuses
Ext:  0xFF
High: 0xD2
Low:  0xFF

`C:\Program Files (x86)\Arduino\hardware\tools\avr\bin> .\avrdude -C ..\etc\avrdude.conf -p m1284p -P com7 -c stk500 -U lfuse:w:0xFF:m -U hfuse:w:0xD2:m -U efuse:w:0xFF:m`


### Firmware




# Bauanleitung

Zuerst den ATmega auflöten, die Markierung (kleiner Punkt) muss zur Beschriftung U1 zeigen.
Danach die Kondensatoren und den Widerstand auflöten.

Mit einem Multimeter messen ob kein Kurzschluss zwischen VCC und GND besteht (mehrere 10 K Widerstand sind okay).

Den ISP Programmieradapter an die Lötaugen des CC1101 anschließen. 

![ISP Anschluss](https://github.com/stan23/HM-ES-PMSw1-Pl_GosundSP1/blob/master/Bilder/Platine_ISP_Beschriftung.jpg)

Pin am ISP-Kabel | Bedeutung
---------------- | ----------
1                | MISO
2                | VCC
3                | SCK
4                | MOSI
5                | Reset
6                | GND



Anschließend kann das Funkmodul auf der Rückseite aufgelötet werden. Dabei gibt es verschiedene Möglichkeiten:
- mit Stift- und Buchsenleiste (abnehmbar)
- mit Stiftleiste

An den Antennenanschluss muss noch das 8,3 cm Drahtstück angelötet werden.

Mit einem FTDI USB-seriell-Adapter an den Lötpunkten RX, TX, 3.3 V und GND und mit einem Terminalprogramm kann man die Ausgabe beobachten. Es sollten schon *Ignore...*-Meldungen empfangen und angezeigt werden.




# Bilder



[![Creative Commons Lizenzvertrag](https://i.creativecommons.org/l/by-nc-sa/3.0/88x31.png)](http://creativecommons.org/licenses/by-nc-sa/3.0/)

Dieses Werk ist lizenziert unter einer [Creative Commons Namensnennung - Nicht-kommerziell - Weitergabe unter gleichen Bedingungen 3.0 International Lizenz](http://creativecommons.org/licenses/by-nc-sa/3.0/).
