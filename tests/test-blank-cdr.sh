#!/bin/bash

cdemu create-blank --writer-id=WRITER-TOC --medium-type=cdr80 --param="writer.write_raw=1" --param="writer.write_subchannel=1" 0 ~/virt.toc
