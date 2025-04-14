# Development

```{include} ../../CONTRIBUTING.md
```

## Compiling the source code

1. Install [Arduino CLI](https://arduino.github.io/arduino-cli/1.2/installation/).
2. Go to the repository root folder
3. Compile and flash the firmware

```shell
arduino-cli compile --clean -e -p COM_PORT -u ats-mini
```

## Compile-time options

The available options are:

* `DISABLE_REMOTE` - disable remote control over the USB-serial port
* `THEME_EDITOR` - enable the color theme editor
* `ENABLE_HOLDOFF` - enable delayed screen update while tuning

To set an option, add the `--build-property` command line argument like this:

```shell
arduino-cli compile --build-property "compiler.cpp.extra_flags=-DTHEME_EDITOR -DENABLE_HOLDOFF" --clean -e -p COM_PORT -u ats-mini
```

## Using the make command

You can do all of the above using the `make` command as well:

```shell
THEME_EDITOR=1 ENABLE_HOLDOFF=1 PORT=/dev/tty.usbmodem14401 make upload
```
