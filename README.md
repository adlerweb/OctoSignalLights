# OctoSignalLights
*OctoSignalLights* is a small script for *ESP8266* based microcontrollers to poll an *OctoPrint* instance via API and display the current print status on a signal light column.

![Example circuit](https://github.com/adlerweb/OctoSignalLights/blob/master/doc/blink.gif?raw=true)

## Pinout & Displays

| Color  | Pin | Static                           | Blink                  |
|--------|-----|----------------------------------|------------------------|
| Red    | D1  | -                                | Printer Error          |
| Yellow | D2  | Printer hot (Tool0 or Bed >40°C) | Printer is heating up  |
| Green  | D3  | Printer online                   | -                      |
| Blue   | D4  | Printing                         | Job paused             |
| White  | D5  | Connection Problem               | Authentication Problem |
| Beeper | D6  | Printer Error                    | -                      |
