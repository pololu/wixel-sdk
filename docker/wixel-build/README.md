# Wixel build action

This action provides a docker container with the SDCC build tools to compile the Wixel firmware.

## Inputs

No inputs required.

## Outputs

No outputs generated.

## Example usage

uses: actions/wixel-build@v1

## Local development

Docker image to build Wixel firmware.

```bash
docker build -t wixel-build .
```

Running image interactively

```bash
docker run -v $(pwd)/../..:/home -it --rm wixel-build sh
```

TODO: Running image with serial port mapped in container

```bash
docker run \
  --privileged \
  -v $(pwd)/../..:/home -it --rm wixel-build sh
make load_example_blink_led
```
