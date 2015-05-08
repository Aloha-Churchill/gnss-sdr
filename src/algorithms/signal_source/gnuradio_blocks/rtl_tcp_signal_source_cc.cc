/*!
 * \file rtl_tcp_signal_source_cc.cc
 * \brief An rtl_tcp signal source reader.
 * \author Anthony Arnold, 2015. anthony.arnold(at)uqconnect.edu.au
 *
 * -------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2015  (see AUTHORS file for a list of contributors)
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
 * along with GNSS-SDR. If not, see <http://www.gnu.org/licenses/>.
 *
 * -------------------------------------------------------------------------
 */

#include "rtl_tcp_signal_source_cc.h"
#include <glog/logging.h>
#include <boost/thread.hpp>

using google::LogMessage;

namespace ip = boost::asio::ip;
using boost::asio::ip::tcp;

// command ids
enum {
   CMD_ID_SET_FREQUENCY = 1,
   CMD_ID_SET_SAMPLE_RATE = 2
};

// rtl_tcp command
struct command {
   enum {
      data_size = 1 + sizeof (unsigned int)
   };
   boost::array<unsigned char, data_size> data_;

   command (unsigned char cmd, unsigned int param)
   {
      data_[0] = cmd;
      unsigned int nparam =
	 boost::asio::detail::socket_ops::host_to_network_long (param);
      memcpy (&data_[1], &nparam, sizeof (nparam));
   }

   boost::system::error_code send (tcp::socket &socket_) {
      boost::system::error_code ec;
      socket_.send (boost::asio::buffer (data_), 0, ec);
      return ec;
   }
};

// set frequency command
struct set_frequency_command : command {
   set_frequency_command (unsigned int freq)
      : command (CMD_ID_SET_FREQUENCY, freq)
   {
   }
};

// set sample rate  command
struct set_sample_rate_command : command {
   set_sample_rate_command (unsigned int sample_rate)
      : command (CMD_ID_SET_SAMPLE_RATE, sample_rate)
   {
   }
};

rtl_tcp_signal_source_cc_sptr
rtl_tcp_make_signal_source_cc(const std::string &address,
			      short port)
{
   return gnuradio::get_initial_sptr (new rtl_tcp_signal_source_cc (address, port));
}


rtl_tcp_signal_source_cc::rtl_tcp_signal_source_cc(const std::string &address,
						   short port)
   : gr::sync_block ("rtl_tcp_signal_source_cc",
                   gr::io_signature::make(0, 0, 0),
                   gr::io_signature::make(1, 1, sizeof(float))),
     socket_ (io_service_),
     buffer_ (1048576),
     unread_ (0)
{
   boost::system::error_code ec;

   for (int i = 0; i < 256; i++) {
      lookup_[i] = (((float)(i & 0xff)) - 127.4f) * (1.0f / 128.0f);
   }

   ip::address addr = ip::address::from_string (address, ec);
   if (ec) {
      std::cout << address << " is not an IP address" << std::endl;
      LOG (WARNING) << address << " is not an IP address";
      return;
   }

   socket_.connect(tcp::endpoint (addr, port), ec);
   if (ec) {
      std::cout << "Failed to connect to " << addr << ":" << port
		<< "(" << ec << ")" << std::endl;
      LOG (WARNING)  << "Failed to connect to " << addr << ":" << port
		   << "(" << ec << ")";
      return;
   }
   std::cout << "Connected to " << addr << ":" << port << std::endl;
   LOG (WARNING)  << "Connected to " << addr << ":" << port;

   boost::asio::async_read (socket_, boost::asio::buffer (data_),
			    boost::bind (&rtl_tcp_signal_source_cc::handle_read,
					 this, _1, _2));
   boost::thread (boost::bind (&boost::asio::io_service::run, &io_service_));
   //io_service_.poll ();
}

rtl_tcp_signal_source_cc::~rtl_tcp_signal_source_cc()
{
   io_service_.stop ();
}

int rtl_tcp_signal_source_cc::work (int noutput_items,
				    gr_vector_const_void_star &input_items,
				    gr_vector_void_star &output_items)
{
   float *out = reinterpret_cast <float *>( output_items[0] );
   int i = 0;

   {
      boost::mutex::scoped_lock lock (mutex_);
      not_empty_.wait (lock, boost::bind (&rtl_tcp_signal_source_cc::not_empty,
					  this));

      for ( ; i < noutput_items && unread_ > 0; i++ ) {
	 out[i] = buffer_[--unread_];
      }
   }
   not_full_.notify_one ();
   return i == 0 ? -1 : i;
}


void rtl_tcp_signal_source_cc::set_frequency (int frequency) {
   boost::system::error_code ec =
      set_frequency_command (frequency).send(socket_);
   if (ec) {
      std::cout << "Failed to set frequency" << std::endl;
      LOG (WARNING) << "Failed to set frequency";
   }
}

void rtl_tcp_signal_source_cc::set_sample_rate (int sample_rate) {
   boost::system::error_code ec =
      set_sample_rate_command (sample_rate).send(socket_);
   if (ec) {
      std::cout << "Failed to set sample rate" << std::endl;
      LOG (WARNING) << "Failed to set sample rate";
   }
}

void rtl_tcp_signal_source_cc::handle_read  (const boost::system::error_code &ec,
					     size_t bytes_transferred)
{
   if (ec) {
      std::cout << "Error during read: " << ec << std::endl;
      LOG (WARNING) << "Error during read: " << ec;
      not_empty_.notify_one ();
   }
   else {
      {
	 boost::mutex::scoped_lock lock (mutex_);
	 not_full_.wait (lock, boost::bind (&rtl_tcp_signal_source_cc::not_full,
					    this));

	 for (size_t i = 0; i < bytes_transferred; i++) {
	    while (!not_full( )) {
	       not_empty_.notify_one ();
	       not_full_.wait (lock, boost::bind (&rtl_tcp_signal_source_cc::not_full,
						  this));
	    }

	    buffer_.push_front (lookup_ [data_[i]]);
	    unread_++;
	 }
      }
      not_empty_.notify_one ();

      boost::asio::async_read (socket_, boost::asio::buffer (data_),
			       boost::bind (&rtl_tcp_signal_source_cc::handle_read,
					    this, _1, _2));
   }
}
