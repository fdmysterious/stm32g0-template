=============================================
Simple C template for STM32 microcontrollerss
=============================================

:Authors:  - Florian Dupeyron <florian.dupeyron@mugcat.fr>
:Date:     April 2022

Build requirements
==================

- A linux build environment
- Docker
- stlink-tools

Build procedure
===============

1. Init and fetch external dependencies:

.. code:: bash

   git submodule update --init

2. Launch the build

.. code:: bash

   ./build.sh

That should be all. You can flash on the target board with the `flash.sh` script.
