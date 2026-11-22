FROM debian:bullseye-slim
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake python3 ca-certificates patchelf binutils \
    libx11-dev libxrandr-dev libgl1-mesa-dev libpng-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
