# dockerfile for us2 compiles

### exposed ARGs override defaults with --build-arg <varname>=<value>

ARG image=ubuntu:12.04
## ARGs are scoped FROM
FROM $image

## compile defaults
ARG parallel_compile=16

ARG DEBIAN_FRONTEND=noninteractive

# update sources.list for old repo
RUN sed -i s/archive.ubuntu/old-releases.ubuntu/ /etc/apt/sources.list

RUN apt-get -y update
RUN apt-get -y install git subversion
RUN apt-get -y install wget bzip2 curl
RUN apt-get -y install build-essential
RUN apt-get -y install qt3-dev-tools
RUN apt-get -y install libqt3-mt-mysql libqt3-mt-psql libqca2-plugin-ossl

# convenience

RUN git clone https://github.com/ehb54/convenience && bash convenience/installcont.sh
RUN apt-get -y install emacs23-nox

# get qwt & make qwt
RUN wget --no-check-certificate https://versaweb.dl.sourceforge.net/project/qwt/qwt/4.2.0/qwt-4.2.0.tar.bz2 && tar jxf qwt-4.2.0.tar.bz2 && cd qwt-4.2.0 && qmake && make -j$parallel_compile

# get qwtplot3d & make
RUN wget --no-check-certificate https://versaweb.dl.sourceforge.net/project/qwtplot3d/qwtplot3d/0.2.7/qwtplot3d-0.2.7.tgz && tar zxf qwtplot3d-0.2.7.tgz && cd qwtplot3d && qmake && make -j$parallel_compile

# get us2
RUN svn co https://github.com/ehb54/ultrascan2/trunk ultrascan2

ENV QWTDIR=/qwt-4.2.0
ENV QWT3DDIR=/qwtplot3d
RUN ln -s /qwt-4.2.0/lib /qwt-4.2.0/lib64
RUN ln -s /qwtplot3d/lib /qwtplot3d/lib64

RUN sed -i 2d ~/.emacs

# build us2

RUN cd /ultrascan2/develop && sh revision.sh && qmake libus && make -j$parallel_compile
ENV ULTRASCAN=/ultrascan2
ENV LD_LIBRARY_PATH=$ULTRASCAN/lib64:$QWTDIR/lib64:$QWT3DDIR/lib64
RUN cd /ultrascan2/develop && qmake us_all && make -j$parallel_compile
ENV LD_LIBRARY_PATH=$ULTRASCAN/lib64:$QWTDIR/lib64:$QWT3DDIR/lib64
ENV PATH=$ULTRASCAN/bin64:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
