/*
 * Copyright (c) 2018, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "gtest/gtest.h"
#include <string.h>
#include "ATHandler_stub.h"
#include "EventQueue.h"
#include "FileHandle_stub.h"
#include "AT_CellularBase_stub.h"
#include "ATHandler.h"
#include "AT_CellularInformation.h"
#include "AT_CellularBase.h"

using namespace mbed;
using namespace events;

// AStyle ignored as the definition is not clear due to preprocessor usage
// *INDENT-OFF*
class TestAT_CellularInformation : public testing::Test {
protected:

    void SetUp()
    {
    }

    void TearDown()
    {
    }
};
// *INDENT-ON*

TEST_F(TestAT_CellularInformation, Create)
{
    EventQueue eq;
    FileHandle_stub fh;
    ATHandler ah(&fh, eq, 0, ",");
    AT_CellularInformation *unit = new AT_CellularInformation(ah);
    EXPECT_TRUE(unit != NULL);
    delete unit;
}

TEST_F(TestAT_CellularInformation, test_AT_CellularInformation_get_manufacturer)
{
    EventQueue eq;
    FileHandle_stub fh;
    ATHandler ah(&fh, eq, 0, ",");
    AT_CellularInformation aci(ah);

    ATHandler_stub::nsapi_error_value = NSAPI_ERROR_OK;
    ATHandler_stub::read_string_value = (char *)"some";
    ATHandler_stub::ssize_value = 4;

    char buf[8];
    EXPECT_TRUE(NSAPI_ERROR_OK == aci.get_manufacturer(buf, 8));
    EXPECT_TRUE(strcmp("some", buf) == 0);

    ATHandler_stub::nsapi_error_value = NSAPI_ERROR_DEVICE_ERROR;
    ATHandler_stub::ssize_value = -1;
    buf[0] = 0;
    EXPECT_TRUE(NSAPI_ERROR_DEVICE_ERROR == aci.get_manufacturer(buf, 8));
    EXPECT_TRUE(strlen(buf) == 0);
}

TEST_F(TestAT_CellularInformation, test_AT_CellularInformation_get_model)
{
    EventQueue eq;
    FileHandle_stub fh;
    ATHandler ah(&fh, eq, 0, ",");
    AT_CellularInformation aci(ah);

    ATHandler_stub::nsapi_error_value = NSAPI_ERROR_OK;
    ATHandler_stub::read_string_value = (char *)"model";
    ATHandler_stub::ssize_value = 5;
    char buf[8];
    EXPECT_TRUE(NSAPI_ERROR_OK == aci.get_model(buf, 8));
    EXPECT_TRUE(strcmp("model", buf) == 0);

    ATHandler_stub::nsapi_error_value = NSAPI_ERROR_DEVICE_ERROR;
    ATHandler_stub::ssize_value = -1;
    buf[0] = 0;
    EXPECT_TRUE(NSAPI_ERROR_DEVICE_ERROR == aci.get_model(buf, 8));
    EXPECT_TRUE(strlen(buf) == 0);
}

TEST_F(TestAT_CellularInformation, test_AT_CellularInformation_get_revision)
{
    EventQueue eq;
    FileHandle_stub fh;
    ATHandler ah(&fh, eq, 0, ",");

    //Used heap var here to visit heap constructor
    AT_CellularInformation *aci = new AT_CellularInformation(ah);

    ATHandler_stub::nsapi_error_value = NSAPI_ERROR_OK;
    ATHandler_stub::read_string_value = (char *)"revision";
    ATHandler_stub::ssize_value = 8;

    char buf[9];
    EXPECT_TRUE(NSAPI_ERROR_OK == aci->get_revision(buf, 9));
    EXPECT_TRUE(strcmp("revision", buf) == 0);

    ATHandler_stub::nsapi_error_value = NSAPI_ERROR_DEVICE_ERROR;
    ATHandler_stub::ssize_value = -1;
    buf[0] = 0;
    EXPECT_TRUE(NSAPI_ERROR_DEVICE_ERROR == aci->get_revision(buf, 8));
    EXPECT_TRUE(strlen(buf) == 0);

    delete aci;
}

TEST_F(TestAT_CellularInformation, test_AT_CellularInformation_get_serial_number)
{
    EventQueue eq;
    FileHandle_stub fh;
    ATHandler ah(&fh, eq, 0, ",");
    AT_CellularInformation aci(ah);

    ATHandler_stub::nsapi_error_value = NSAPI_ERROR_OK;
    ATHandler_stub::read_string_value = (char *)"1234567";
    ATHandler_stub::ssize_value = 7;
    char buf[8];

    EXPECT_TRUE(NSAPI_ERROR_OK == aci.get_serial_number(buf, 8, CellularInformation::SN));
    EXPECT_TRUE(strcmp("1234567", buf) == 0);

    ATHandler_stub::nsapi_error_value = NSAPI_ERROR_DEVICE_ERROR;
    ATHandler_stub::ssize_value = -1;
    buf[0] = 0;
    EXPECT_TRUE(NSAPI_ERROR_DEVICE_ERROR == aci.get_serial_number(buf, 8, CellularInformation::SN));
    EXPECT_TRUE(strlen(buf) == 0);

    AT_CellularBase_stub::supported_bool = false;
    EXPECT_TRUE(NSAPI_ERROR_UNSUPPORTED == aci.get_serial_number(buf, 8, CellularInformation::IMEI));
    EXPECT_TRUE(strlen(buf) == 0);

    ATHandler_stub::nsapi_error_value = NSAPI_ERROR_OK;
    ATHandler_stub::read_string_value = (char *)"1234567";
    ATHandler_stub::ssize_value = 7;
    AT_CellularBase_stub::supported_bool = true;
    EXPECT_TRUE(NSAPI_ERROR_OK == aci.get_serial_number(buf, 8, CellularInformation::IMEI));
    EXPECT_TRUE(strcmp("1234567", buf) == 0);
}
