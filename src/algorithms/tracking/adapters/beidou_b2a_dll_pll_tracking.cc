/*!
 * \file beidou_b2a_dll_pll_tracking.cc
 * \brief  Interface of an adapter of a DLL+PLL tracking loop block
 * for BEIDOU B2A to a TrackingInterface
 * \author Sara Hrbek, 2018. sara.hrbek(at)gmail.com gsoc 2018; Aloha Churchill, 2022 as part of the 2022 GSoC Program
 *
 * Code DLL + carrier PLL according to the algorithms described in:
 * K.Borre, D.M.Akos, N.Bertelsen, P.Rinder, and S.H.Jensen,
 * A Software-Defined GPS and Galileo Receiver. A Single-Frequency
 * Approach, Birkhauser, 2007
 *
 * -------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2018  (see AUTHORS file for a list of contributors)
 *
 * GNSS-SDR is a software defined Global Navigation
 *          Satellite Systems receiver
 *
 * This file is part of GNSS-SDR.
 *
 * GNSS-SDR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GNSS-SDR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNSS-SDR. If not, see <https://www.gnu.org/licenses/>.
 *
 * -------------------------------------------------------------------------
 */

#include "beidou_b2a_dll_pll_tracking.h"
#include "Beidou_B2a.h"
#include "configuration_interface.h"
#include "display.h"
#include "dll_pll_conf.h"
#include "gnss_sdr_flags.h"
#include <glog/logging.h>


BeidouB2aDllPllTracking::BeidouB2aDllPllTracking(
    const ConfigurationInterface* configuration, const std::string& role,
    unsigned int in_streams, unsigned int out_streams) : role_(role), in_streams_(in_streams), out_streams_(out_streams)
{
    Dll_Pll_Conf trk_param = Dll_Pll_Conf();
    DLOG(INFO) << "role " << role;

    
    trk_param.SetFromConfiguration(configuration, role);
    
    const auto vector_length = static_cast<int>(std::round(trk_param.fs_in / (BEIDOU_B2ap_CODE_RATE_HZ / BEIDOU_B2ap_CODE_LENGTH_CHIPS)));
    trk_param.vector_length = vector_length;
    if (trk_param.extend_correlation_symbols < 1)
        {
            trk_param.extend_correlation_symbols = 1;
            std::cout << TEXT_RED << "WARNING: BeiDou B2a. extend_correlation_symbols must be bigger than 0. Coherent integration has been set to 1 symbol (1 ms)" << TEXT_RESET << std::endl;
        }
    else if (!trk_param.track_pilot and trk_param.extend_correlation_symbols > BEIDOU_B2ad_SECONDARY_CODE_LENGTH)
        {
            trk_param.extend_correlation_symbols = BEIDOU_B2ad_SECONDARY_CODE_LENGTH;
            std::cout << TEXT_RED << "WARNING: BeiDou B2a. extend_correlation_symbols must be lower than 5 when tracking the data component. Coherent integration has been set to 10 symbols (10 ms)" << TEXT_RESET << std::endl;
        }
    if ((trk_param.extend_correlation_symbols > 1) and (trk_param.pll_bw_narrow_hz > trk_param.pll_bw_hz or trk_param.dll_bw_narrow_hz > trk_param.dll_bw_hz))
        {
            std::cout << TEXT_RED << "WARNING: BeiDou B2a. PLL or DLL narrow tracking bandwidth is higher than wide tracking one" << TEXT_RESET << std::endl;
        }

    trk_param.system = 'C';
    const std::array<char, 3> sig_{'5', 'C', '\0'};
    std::memcpy(trk_param.signal, sig_.data(), 3);

    // ################# Make a GNU Radio Tracking block object ################
    if (trk_param.item_type == "gr_complex")
        {
            item_size_ = sizeof(gr_complex);
            tracking_ = dll_pll_veml_make_tracking(trk_param);
        }
    else
        {
            item_size_ = 0;
            LOG(WARNING) << trk_param.item_type << " unknown tracking item type.";
        }
    channel_ = 0;
    DLOG(INFO) << "tracking(" << tracking_->unique_id() << ")";
    if (in_streams_ > 1)
        {
            LOG(ERROR) << "This implementation only supports one input stream";
        }
    if (out_streams_ > 1)
        {
            LOG(ERROR) << "This implementation only supports one output stream";
        }

}



void BeidouB2aDllPllTracking::start_tracking()
{
    tracking_->start_tracking();
}


void BeidouB2aDllPllTracking::stop_tracking()
{
    tracking_->stop_tracking();
}


/*
 * Set tracking channel unique ID
 */
void BeidouB2aDllPllTracking::set_channel(unsigned int channel)
{
    channel_ = channel;
    tracking_->set_channel(channel);
}


void BeidouB2aDllPllTracking::set_gnss_synchro(Gnss_Synchro* p_gnss_synchro)
{
    tracking_->set_gnss_synchro(p_gnss_synchro);
}


void BeidouB2aDllPllTracking::connect(gr::top_block_sptr top_block)
{
    if (top_block)
        { /* top_block is not null */
        };
    //nothing to connect, now the tracking uses gr_sync_decimator
}


void BeidouB2aDllPllTracking::disconnect(gr::top_block_sptr top_block)
{
    if (top_block)
        { /* top_block is not null */
        };
    //nothing to disconnect, now the tracking uses gr_sync_decimator
}


gr::basic_block_sptr BeidouB2aDllPllTracking::get_left_block()
{
    return tracking_;
}


gr::basic_block_sptr BeidouB2aDllPllTracking::get_right_block()
{
    return tracking_;
}