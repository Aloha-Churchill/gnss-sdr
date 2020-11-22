/*!
 * \file tlm_conf.cc
 * \brief Class that contains all the configuration parameters for generic
 * telemetry decoder block.
 * \author Carles Fernandez, 2020. cfernandez(at)cttc.es
 *
 * -----------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2020  (see AUTHORS file for a list of contributors)
 *
 * GNSS-SDR is a software defined Global Navigation
 *          Satellite Systems receiver
 *
 * This file is part of GNSS-SDR.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * -----------------------------------------------------------------------------
 */

#include "tlm_conf.h"

Tlm_Conf::Tlm_Conf()
{
    dump = false;
    dump_mat = false;
}


void Tlm_Conf::SetFromConfiguration(const ConfigurationInterface *configuration,
    const std::string &role)
{
    const std::string default_dumpname("telemetry");
    dump_filename = configuration->property(role + ".dump_filename", default_dumpname);
    dump = configuration->property(role + ".dump", false);
    dump_mat = configuration->property(role + ".dump_mat", dump);
}