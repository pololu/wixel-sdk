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

Updating docker image at ghcr.io

```
export CR_PAT=YOUR_TOKEN
echo $CR_PAT | docker login ghcr.io -u USERNAME --password-stdin
docker tag wixel-build:latest ghcr.io/USERNAME/wixel-build:latest
```