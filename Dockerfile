from alpine:latest

RUN apk add gcc-arm-none-eabi
RUN apk add ninja
RUN apk add re2c
RUN apk add python3
RUN apk add cmake
RUN apk add newlib-arm-none-eabi
