# SPDX-FileCopyrightText: 2026 ARC2026 Project
# SPDX-License-Identifier: CC0-1.0
import pytest
from pytest_embedded_idf.dut import IdfDut
from pytest_embedded_idf.utils import idf_parametrize

@pytest.mark.generic
@idf_parametrize('target', ['esp32'], indirect=['target'])
def test_arc2026_robot_control(dut: IdfDut) -> None:
    dut.expect_exact('=== ESP32 PS4 Robot Control Starting ===')
    dut.expect_exact('Target PS4 Controller MAC: 00:0A:0A:0A:0A:0A')
    dut.expect_exact('Robot control loop ready. Waiting for PS4 Controller connection...')


