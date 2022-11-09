# FEUP-RC-PROJ1

First project for the RC course unit at FEUP.

The objective of the project was to develop an application, separated into two layers, for sending files between two computers by using the serial port.

[Final project report](report/report.pdf)

## Building and running

Assuming make is installed, call `make` to build the project, and `make docs` to generate the documentation.

Call `make run_tx` to run the project with the default transmitter options, and `make run_rx` to run with the default receiver options. Edit the [makefile](Makefile) to change these settings.

If your computer has no serial port, or you want to test locally, call `make run_cable` to create a virtual serial port. The default settings will use this port.

## Unit info

- **Name**: Redes de Computadores (Computer Networks)
- **Date**: Year 3, Semester 1, 2022/23
- **See also**: [feup-rc-proj2](https://github.com/ttoino/feup-rc-proj2)
- [**More info**](https://sigarra.up.pt/feup/ucurr_geral.ficha_uc_view?pv_ocorrencia_id=501687)

## Disclaimer

This repository (and all others with the name format `feup-*`) are for archival and educational purposes only.

If you don't understand some part of the code or anything else in this repo, feel free to ask (although I may not understand it myself anymore).

Keep in mind that this repo is public. If you copy any code and use it in your school projects you may be flagged for plagiarism by automated tools.
