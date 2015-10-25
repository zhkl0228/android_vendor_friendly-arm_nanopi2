# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

VENDOR_PATH := vendor/friendly-arm/nanopi2

# ogl
PRODUCT_COPY_FILES += \
	$(VENDOR_PATH)/prebuilt/vr.ko:system/lib/modules/vr.ko

# coda
PRODUCT_COPY_FILES += \
	$(VENDOR_PATH)/prebuilt/nx_vpu.ko:system/lib/modules/nx_vpu.ko

# ap621x
PRODUCT_COPY_FILES += \
	$(VENDOR_PATH)/prebuilt/bcmdhd.ko:system/lib/modules/bcmdhd.ko

# Prebuilts
#PRODUCT_PACKAGES += \
	prebuilt.busybox \

