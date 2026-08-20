# =========================================================
# ETAPA 1: COMPILACIÓN DEL TOOLCHAIN DE PS3 (PSL1GHT)
# =========================================================
FROM ubuntu:18.04 as ps3toolchain

ENV DEBIAN_FRONTEND=noninteractive

# Instalar dependencias necesarias para compilar el SDK
RUN \
  apt-get -y update && \
  apt-get -y install \
  autoconf bison build-essential ca-certificates flex git libelf-dev\
  libgmp-dev libncurses5-dev libssl-dev libtool-bin pkg-config python python-dev \
  texinfo wget zlib1g-dev && \
  apt-get -y clean autoclean autoremove && \
  rm -rf /var/lib/{apt,dpkg,cache,log}/

# Arreglar errores de certificados antiguos en wget
RUN echo "\nca_certificate=/etc/ssl/certs/ca-certificates.crt" | tee -a /etc/wgetrc

ENV PS3DEV /ps3dev
ENV PSL1GHT ${PS3DEV}
ENV PATH ${PATH}:${PS3DEV}/bin:${PS3DEV}/ppu/bin:${PS3DEV}/spu/bin

# DESCARGAR EL TOOLCHAIN OFICIAL Y COMPILARLO AUTOMATICAMENTE
RUN git clone https://github.com/ps3dev/ps3toolchain.git /build && \
    cd /build && \
    ./toolchain.sh

# =========================================================
# ETAPA 2: CREACIÓN DEL ENTORNO FINAL PARA MINECRAFT PE
# =========================================================
FROM ubuntu:18.04 as build

# Instalar dependencias de compilación estándar
RUN apt-get update && \
  apt-get install -y \
    binutils-mips-linux-gnu \
    bsdmainutils \
    build-essential \
    libaudiofile-dev \
    libelf-dev \
    pkg-config \
    python \
    python-dev \
    python3 \
    wget \
    zlib1g-dev

# Instalar NVIDIA Cg Toolkit (VITAL para compilar los shaders .vcg y .fcg de MCPE)
RUN wget http://developer.download.nvidia.com/cg/Cg_3.1/Cg-3.1_April2012_x86_64.deb && \
  echo '6da24fd6698dbb43ae5eee8691817d88d5792d89e2e8b9acf07597bec35cb080  Cg-3.1_April2012_x86_64.deb' \
    | sha256sum Cg-3.1_April2012_x86_64.deb && \
  dpkg -i Cg-3.1_April2012_x86_64.deb && \
  rm Cg-3.1_April2012_x86_64.deb

# Copiar el SDK ya compilado de la Etapa 1 a esta imagen limpia
COPY --from=ps3toolchain /ps3dev /ps3dev

# Configurar variables de entorno para compilar
ENV PS3DEV /ps3dev
ENV PSL1GHT ${PS3DEV}
ENV PATH ${PATH}:${PS3DEV}/bin:${PS3DEV}/ppu/bin:${PS3DEV}/spu/bin

# Crear y establecer la carpeta de trabajo de Minecraft PE
RUN mkdir /mcpe
WORKDIR /mcpe

# Mensaje de ayuda que se muestra si inicias el contenedor sin comandos
CMD echo '==========================================================\nEntorno Docker para Minecraft PE (PS3) cargado.\nPython 2.7 activado por defecto.\n\nPara abrir la terminal interactiva usa:\ndocker run --rm -it -v $(pwd):/mcpe ps3dev-mcpe /bin/bash\n\nPara compilar el juego directamente usa:\ndocker run --rm -v $(pwd):/mcpe ps3dev-mcpe make -j4\n==========================================================\n'
