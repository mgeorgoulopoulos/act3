FROM debian:bullseye-slim
# Bullseye is archived; its older glibc is intentional for release compatibility.
RUN printf '%s\n' \
    'deb http://archive.debian.org/debian bullseye main' \
    'deb http://archive.debian.org/debian-security bullseye-security main' \
    > /etc/apt/sources.list \
    && rm -f /etc/apt/sources.list.d/* \
    && apt-get -o Acquire::Check-Valid-Until=false update \
    && apt-get install -y --no-install-recommends \
    build-essential cmake python3 ca-certificates patchelf binutils \
    libx11-dev libxrandr-dev libgl1-mesa-dev libpng-dev xvfb xauth \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
