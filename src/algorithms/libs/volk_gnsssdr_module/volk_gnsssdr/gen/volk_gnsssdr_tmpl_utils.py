#!/usr/bin/env python
#
# GNSS-SDR is a Global Navigation Satellite System software-defined receiver.
# This file is part of GNSS-SDR.
#
# Copyright (C) 2010-2020  (see AUTHORS file for a list of contributors)
# SPDX-License-Identifier: GPL-3.0-or-later

from __future__ import print_function

import os
import re
import sys
import optparse
import volk_gnsssdr_arch_defs
import volk_gnsssdr_machine_defs
import volk_gnsssdr_kernel_defs
from mako.template import Template

def __parse_tmpl(_tmpl, **kwargs):
    defs = {
        'archs': volk_gnsssdr_arch_defs.archs,
        'arch_dict': volk_gnsssdr_arch_defs.arch_dict,
        'machines': volk_gnsssdr_machine_defs.machines,
        'machine_dict': volk_gnsssdr_machine_defs.machine_dict,
        'kernels': volk_gnsssdr_kernel_defs.kernels,
    }
    defs.update(kwargs)
    _tmpl = """

/* this file was generated by volk_gnsssdr template utils, do not edit! */

""" + _tmpl
    return str(Template(_tmpl).render(**defs))

def main():
    parser = optparse.OptionParser()
    parser.add_option('--input', type='string')
    parser.add_option('--output', type='string')
    (opts, args) = parser.parse_args()

    output = __parse_tmpl(open(opts.input).read(), args=args)
    if opts.output: open(opts.output, 'w').write(output)
    else: print(output)

if __name__ == '__main__': main()
