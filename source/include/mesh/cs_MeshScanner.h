/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

extern "C" {
#include <nrf_mesh.h>
}

/**
 * Class that handles scans.
 */
class MeshScanner {
public:
	void onScan(const nrf_mesh_adv_packet_rx_data_t *scanData);
private:
	/**
	 * Variable to copy scanned data to, so that it doesn't get created on the stack all the time.
	 */
	scanned_device_t _scannedDevice = {0};
};
