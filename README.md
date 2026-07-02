# esp32-ClawBot project use ESP-IDF toolchain
## EDP-IDF common commands:
| Description                     | Command                                                               |
| ------------------------------- | ----------------------------------------------------------------------|
| Enter idf env                   | source "/home/username/.espressif/tools/activate_idf_v6.0.1.sh"       |
| Set target                      | idf.py set-target esp32s3                                             |
| Enter configuration menu        | idf.py menuconfig                                                     |
| Build                           | idf.py build                                                          |
| Flash                           | idf.py -p /dev/ttyACM0 flash                                          |
| Enter monitor                   | idf.py monitor                                                        |
| Quit monitor                    |  Ctrl + ]                                                             |
| Monitor and logging             | idf.py monitor \| Tee-Object -FilePath log.txt                        |
| Remove build folder             | Remove-Item -Recurse -Force build                                     |
| Full clean                      | idf.py fullclean                                                      |

## lvgl integration

lvgl is not included in ESP-IDF, need add it manually.
```bash
cd components
git clone https://github.com/lvgl/lvgl.git
```
