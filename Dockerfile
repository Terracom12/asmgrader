FROM debian:bookworm

RUN apt-get update -qq && apt-get install -qq --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    g++ \
    gcc \
    g++-aarch64-linux-gnu \
    gcc-aarch64-linux-gnu \
    ccache \
    git \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

ENV CPM_SOURCE_CACHE="/workspace/CPM" \
    CCACHE_DIR="/workspace/ccache" \
    CCACHE_SLOPPINESS="include_file_ctime,include_file_mtime,pch_defines,time_macros" \
    CCACHE_NAMESPACE="asmgrader-aarch64" \
    CCACHE_MAXSIZE="10Gi" \
    CCACHE_PCH_EXTSUM="true"

CMD ["/bin/bash"]
