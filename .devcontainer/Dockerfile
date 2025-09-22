FROM ubuntu:22.04

# Éviter les questions interactives
ENV DEBIAN_FRONTEND=noninteractive

# Installer build tools et debugger
RUN apt-get update && apt-get install -y \
    build-essential \
    gdb \
    clang \
    cmake \
    python3 \
    python3-pip \
    curl \
    git \
    && rm -rf /var/lib/apt/lists/*

# Ajouter un utilisateur "vscode" (non root)
RUN useradd -m vscode && echo "vscode ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER vscode
WORKDIR /workspaces
