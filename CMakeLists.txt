cmake_minimum_required(VERSION 3.22.1)

project("rtlsdr433")

# =============================================================================
# Native Module Layout
# =============================================================================
# This builds a single shared library 'librtlsdr433' composed of:
#
#   ebc-sdr-native/  - git submodule, pinned to a tag. The shared SDR base of all three EBC
#                      SDR apps: libusb 1.0.23 (Android port, unplug fixes), rtl-sdr
#                      (osmocom v2.0.3 plus the RTL-SDR-Blog features worth keeping) and
#                      rtlsdr_open2(dev**, fd), the fd -> rtlsdr_dev_t bridge. It builds
#                      itself as the static library `ebc_sdr` and brings its own include
#                      paths, compile options and linker flags. Provenance of every local
#                      patch: ebc-sdr-native/PROVENANCE.md.
#   rtl433/          - rtl_433 decoding engine (263 433 MHz device decoders, full v25.12)
#   rtlsdr433.cpp    - JNI bridge: exposes start/stop/status to Kotlin NativeBridge
#   android_bridge.c - rtl_433 data_output_t serialising events to JSON for the JNI callback
#
# Output pipeline: rtl_433 decoder -> announce_output_line() [rtlsdr433.cpp]
#                  -> NativeBridge.nativeOutputLine() [JNI callback to Kotlin]
#
# Until v1.3.2 this directory carried its own copies of rtl-sdr/ and libusb-andro/ plus
# librtlsdr_andro.c, and listed every one of their sources below. They are gone: three apps
# each maintaining a copy is what the submodule exists to end. A clone needs
# `git clone --recursive`, or `git submodule update --init` afterwards.
# =============================================================================

set(RTL433_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rtl433/src)

# The shared base, first: everything below inherits this directory's compile definitions and
# include paths, and the library must not -- it has to compile the same way in all three apps.
# Its own log tag keeps rtl-sdr's and libusb's diagnostics apart from rtl_433's in logcat; the
# bridge used to hardcode RTL433_USB.
set(EBC_SDR_LOG_TAG "rtl433-sdr" CACHE STRING "" FORCE)
add_subdirectory(ebc-sdr-native)

# rtl-sdr/include and libusb-andro/libusb are NOT listed here: they arrive as PUBLIC include
# directories of the ebc_sdr target. rtl433/src/sdr.c includes both <rtl-sdr.h> and <libusb.h>
# directly, which is why the library exports them.
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/rtl433/include
    ${CMAKE_CURRENT_SOURCE_DIR}/rtl433
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(rtlsdr433 SHARED
    rtlsdr433.cpp
    android_bridge.c

    # rtl_433 core sources
    ${RTL433_SRC_DIR}/am_analyze.c
    ${RTL433_SRC_DIR}/abuf.c
    ${RTL433_SRC_DIR}/bitbuffer.c
    ${RTL433_SRC_DIR}/baseband.c
    ${RTL433_SRC_DIR}/confparse.c
    ${RTL433_SRC_DIR}/compat_paths.c
    ${RTL433_SRC_DIR}/compat_time.c
    ${RTL433_SRC_DIR}/data.c
    ${RTL433_SRC_DIR}/data_tag.c
    ${RTL433_SRC_DIR}/decoder_util.c
    ${RTL433_SRC_DIR}/fileformat.c
    ${RTL433_SRC_DIR}/http_server.c
    ${RTL433_SRC_DIR}/jsmn.c
    ${RTL433_SRC_DIR}/list.c
    ${RTL433_SRC_DIR}/logger.c
    ${RTL433_SRC_DIR}/mongoose.c
    ${RTL433_SRC_DIR}/optparse.c
    ${RTL433_SRC_DIR}/output_file.c
    ${RTL433_SRC_DIR}/output_influx.c
    ${RTL433_SRC_DIR}/output_log.c
    ${RTL433_SRC_DIR}/output_mqtt.c
    ${RTL433_SRC_DIR}/output_rtltcp.c
    ${RTL433_SRC_DIR}/output_trigger.c
    ${RTL433_SRC_DIR}/output_udp.c
    ${RTL433_SRC_DIR}/pulse_analyzer.c
    ${RTL433_SRC_DIR}/pulse_detect_fsk.c
    ${RTL433_SRC_DIR}/pulse_data.c
    ${RTL433_SRC_DIR}/pulse_slicer.c
    ${RTL433_SRC_DIR}/pulse_detect.c
    ${RTL433_SRC_DIR}/r_util.c
    ${RTL433_SRC_DIR}/bit_util.c
    ${RTL433_SRC_DIR}/rfraw.c
    ${RTL433_SRC_DIR}/r_api.c
    ${RTL433_SRC_DIR}/rtl_433.c
    ${RTL433_SRC_DIR}/raw_output.c
    ${RTL433_SRC_DIR}/sdr.c
    ${RTL433_SRC_DIR}/samp_grab.c
    ${RTL433_SRC_DIR}/term_ctl.c
    ${RTL433_SRC_DIR}/write_sigrok.c

    # rtl_433 devices (all)
    ${RTL433_SRC_DIR}/devices/abmt.c
    ${RTL433_SRC_DIR}/devices/acurite_01185m.c
    ${RTL433_SRC_DIR}/devices/acurite.c
    ${RTL433_SRC_DIR}/devices/akhan_100F14.c
    ${RTL433_SRC_DIR}/devices/alecto.c
    ${RTL433_SRC_DIR}/devices/ambient_weather.c
    ${RTL433_SRC_DIR}/devices/ambientweather_tx8300.c
    ${RTL433_SRC_DIR}/devices/ambientweather_wh31e.c
    ${RTL433_SRC_DIR}/devices/ant_antplus.c
    ${RTL433_SRC_DIR}/devices/apator_metra_eitn30.c
    ${RTL433_SRC_DIR}/devices/apator_metra_erm30.c
    ${RTL433_SRC_DIR}/devices/arad_ms_meter.c
    ${RTL433_SRC_DIR}/devices/archos_tbh.c
    ${RTL433_SRC_DIR}/devices/arexx_ml.c
    ${RTL433_SRC_DIR}/devices/atech_ws308.c
    ${RTL433_SRC_DIR}/devices/auriol_4ld5661.c
    ${RTL433_SRC_DIR}/devices/auriol_aft77b2.c
    ${RTL433_SRC_DIR}/devices/auriol_afw2a1.c
    ${RTL433_SRC_DIR}/devices/auriol_ahfl.c
    ${RTL433_SRC_DIR}/devices/auriol_hg02832.c
    ${RTL433_SRC_DIR}/devices/badger_orion_endpoint.c
    ${RTL433_SRC_DIR}/devices/badger_water.c
    ${RTL433_SRC_DIR}/devices/baldr_rain.c
    ${RTL433_SRC_DIR}/devices/baldr_therm.c
    ${RTL433_SRC_DIR}/devices/blueline.c
    ${RTL433_SRC_DIR}/devices/blyss.c
    ${RTL433_SRC_DIR}/devices/bm5.c
    ${RTL433_SRC_DIR}/devices/brennenstuhl_rcs_2044.c
    ${RTL433_SRC_DIR}/devices/bresser_3ch.c
    ${RTL433_SRC_DIR}/devices/bresser_5in1.c
    ${RTL433_SRC_DIR}/devices/bresser_6in1.c
    ${RTL433_SRC_DIR}/devices/bresser_7in1.c
    ${RTL433_SRC_DIR}/devices/bresser_leakage.c
    ${RTL433_SRC_DIR}/devices/bresser_lightning.c
    ${RTL433_SRC_DIR}/devices/bresser_st1005h.c
    ${RTL433_SRC_DIR}/devices/bt_rain.c
    ${RTL433_SRC_DIR}/devices/burnhardbbq.c
    ${RTL433_SRC_DIR}/devices/calibeur.c
    ${RTL433_SRC_DIR}/devices/cardin.c
    ${RTL433_SRC_DIR}/devices/cavius.c
    ${RTL433_SRC_DIR}/devices/ced7000.c
    ${RTL433_SRC_DIR}/devices/celsia_czc1.c
    ${RTL433_SRC_DIR}/devices/chamberlain_cwpirc.c
    ${RTL433_SRC_DIR}/devices/chuango.c
    ${RTL433_SRC_DIR}/devices/cmr113.c
    ${RTL433_SRC_DIR}/devices/companion_wtr001.c
    ${RTL433_SRC_DIR}/devices/cotech_36_7959.c
    ${RTL433_SRC_DIR}/devices/current_cost.c
    ${RTL433_SRC_DIR}/devices/danfoss.c
    ${RTL433_SRC_DIR}/devices/deltadore_x3d.c
    ${RTL433_SRC_DIR}/devices/digitech_xc0324.c
    ${RTL433_SRC_DIR}/devices/directv.c
    ${RTL433_SRC_DIR}/devices/dish_remote_6_3.c
    ${RTL433_SRC_DIR}/devices/dsc.c
    ${RTL433_SRC_DIR}/devices/ec3k.c
    ${RTL433_SRC_DIR}/devices/ecodhome.c
    ${RTL433_SRC_DIR}/devices/ecowitt.c
    ${RTL433_SRC_DIR}/devices/efergy_e2_classic.c
    ${RTL433_SRC_DIR}/devices/efergy_optical.c
    ${RTL433_SRC_DIR}/devices/efth800.c
    ${RTL433_SRC_DIR}/devices/elro_db286a.c
    ${RTL433_SRC_DIR}/devices/elv.c
    ${RTL433_SRC_DIR}/devices/emax.c
    ${RTL433_SRC_DIR}/devices/emontx.c
    ${RTL433_SRC_DIR}/devices/emos_e6016_rain.c
    ${RTL433_SRC_DIR}/devices/emos_e6016.c
    ${RTL433_SRC_DIR}/devices/enocean_erp1.c
    ${RTL433_SRC_DIR}/devices/ert_idm.c
    ${RTL433_SRC_DIR}/devices/ert_scm.c
    ${RTL433_SRC_DIR}/devices/esa.c
    ${RTL433_SRC_DIR}/devices/esic_emt7110.c
    ${RTL433_SRC_DIR}/devices/esperanza_ews.c
    ${RTL433_SRC_DIR}/devices/eurochron.c
    ${RTL433_SRC_DIR}/devices/fineoffset_wh1050.c
    ${RTL433_SRC_DIR}/devices/fineoffset_wh1080.c
    ${RTL433_SRC_DIR}/devices/fineoffset_wh31l.c
    ${RTL433_SRC_DIR}/devices/fineoffset_wh43.c
    ${RTL433_SRC_DIR}/devices/fineoffset_wh45.c
    ${RTL433_SRC_DIR}/devices/fineoffset_wh46.c
    ${RTL433_SRC_DIR}/devices/fineoffset_wh55.c
    ${RTL433_SRC_DIR}/devices/fineoffset_wn34.c
    ${RTL433_SRC_DIR}/devices/fineoffset_ws80.c
    ${RTL433_SRC_DIR}/devices/fineoffset_ws85.c
    ${RTL433_SRC_DIR}/devices/fineoffset_ws90.c
    ${RTL433_SRC_DIR}/devices/fineoffset.c
    ${RTL433_SRC_DIR}/devices/flex.c
    ${RTL433_SRC_DIR}/devices/flowis.c
    ${RTL433_SRC_DIR}/devices/fordremote.c
    ${RTL433_SRC_DIR}/devices/fs20.c
    ${RTL433_SRC_DIR}/devices/ft004b.c
    ${RTL433_SRC_DIR}/devices/funkbus.c
    ${RTL433_SRC_DIR}/devices/gasmate_ba1008.c
    ${RTL433_SRC_DIR}/devices/ge_coloreffects.c
    ${RTL433_SRC_DIR}/devices/geevon_tx19.c
    ${RTL433_SRC_DIR}/devices/geevon.c
    ${RTL433_SRC_DIR}/devices/generic_motion.c
    ${RTL433_SRC_DIR}/devices/generic_remote.c
    ${RTL433_SRC_DIR}/devices/generic_temperature_sensor.c
    ${RTL433_SRC_DIR}/devices/geo_minim.c
    ${RTL433_SRC_DIR}/devices/govee_h5059.c
    ${RTL433_SRC_DIR}/devices/govee.c
    ${RTL433_SRC_DIR}/devices/gridstream.c
    ${RTL433_SRC_DIR}/devices/gt_tmbbq05.c
    ${RTL433_SRC_DIR}/devices/gt_wt_02.c
    ${RTL433_SRC_DIR}/devices/gt_wt_03.c
    ${RTL433_SRC_DIR}/devices/hcs200.c
    ${RTL433_SRC_DIR}/devices/hideki.c
    ${RTL433_SRC_DIR}/devices/holman_ws5029.c
    ${RTL433_SRC_DIR}/devices/homelead_hg9901.c
    ${RTL433_SRC_DIR}/devices/hondaremote.c
    ${RTL433_SRC_DIR}/devices/honeywell_cm921.c
    ${RTL433_SRC_DIR}/devices/honeywell_wdb.c
    ${RTL433_SRC_DIR}/devices/honeywell.c
    ${RTL433_SRC_DIR}/devices/ht680.c
    ${RTL433_SRC_DIR}/devices/ibis_beacon.c
    ${RTL433_SRC_DIR}/devices/ikea_sparsnas.c
    ${RTL433_SRC_DIR}/devices/infactory.c
    ${RTL433_SRC_DIR}/devices/inkbird_ith20r.c
    ${RTL433_SRC_DIR}/devices/inovalley-kw9015b.c
    ${RTL433_SRC_DIR}/devices/insteon.c
    ${RTL433_SRC_DIR}/devices/interlogix.c
    ${RTL433_SRC_DIR}/devices/intertechno.c
    ${RTL433_SRC_DIR}/devices/jasco.c
    ${RTL433_SRC_DIR}/devices/kedsum.c
    ${RTL433_SRC_DIR}/devices/kerui.c
    ${RTL433_SRC_DIR}/devices/klimalogg.c
    ${RTL433_SRC_DIR}/devices/lacrosse_breezepro.c
    ${RTL433_SRC_DIR}/devices/lacrosse_r1.c
    ${RTL433_SRC_DIR}/devices/lacrosse_th3.c
    ${RTL433_SRC_DIR}/devices/lacrosse_tx141x.c
    ${RTL433_SRC_DIR}/devices/lacrosse_tx31u.c
    ${RTL433_SRC_DIR}/devices/lacrosse_tx34.c
    ${RTL433_SRC_DIR}/devices/lacrosse_tx35.c
    ${RTL433_SRC_DIR}/devices/lacrosse_wr1.c
    ${RTL433_SRC_DIR}/devices/lacrosse_ws7000.c
    ${RTL433_SRC_DIR}/devices/lacrosse.c
    ${RTL433_SRC_DIR}/devices/lacrossews.c
    ${RTL433_SRC_DIR}/devices/lightwave_rf.c
    ${RTL433_SRC_DIR}/devices/m_bus.c
    ${RTL433_SRC_DIR}/devices/markisol.c
    ${RTL433_SRC_DIR}/devices/marlec_solar.c
    ${RTL433_SRC_DIR}/devices/maverick_et73.c
    ${RTL433_SRC_DIR}/devices/maverick_et73x.c
    ${RTL433_SRC_DIR}/devices/maverick_xr30.c
    ${RTL433_SRC_DIR}/devices/maverick_xr50.c
    ${RTL433_SRC_DIR}/devices/mebus.c
    ${RTL433_SRC_DIR}/devices/megacode.c
    ${RTL433_SRC_DIR}/devices/missil_ml0757.c
    ${RTL433_SRC_DIR}/devices/mueller_hotrod.c
    ${RTL433_SRC_DIR}/devices/neptune_r900.c
    ${RTL433_SRC_DIR}/devices/new_template.c
    ${RTL433_SRC_DIR}/devices/newkaku.c
    ${RTL433_SRC_DIR}/devices/nexa.c
    ${RTL433_SRC_DIR}/devices/nexus.c
    ${RTL433_SRC_DIR}/devices/nice_flor_s.c
    ${RTL433_SRC_DIR}/devices/norgo.c
    ${RTL433_SRC_DIR}/devices/oil_smart.c
    ${RTL433_SRC_DIR}/devices/oil_standard.c
    ${RTL433_SRC_DIR}/devices/oil_watchman_advanced.c
    ${RTL433_SRC_DIR}/devices/oil_watchman.c
    ${RTL433_SRC_DIR}/devices/opus_xt300.c
    ${RTL433_SRC_DIR}/devices/oregon_scientific_sl109h.c
    ${RTL433_SRC_DIR}/devices/oregon_scientific_v1.c
    ${RTL433_SRC_DIR}/devices/oregon_scientific.c
    ${RTL433_SRC_DIR}/devices/oria_wa150km.c
    ${RTL433_SRC_DIR}/devices/philips_aj3650.c
    ${RTL433_SRC_DIR}/devices/philips_aj7010.c
    ${RTL433_SRC_DIR}/devices/proflame2.c
    ${RTL433_SRC_DIR}/devices/prologue.c
    ${RTL433_SRC_DIR}/devices/proove.c
    ${RTL433_SRC_DIR}/devices/quhwa.c
    ${RTL433_SRC_DIR}/devices/quinetic.c
    ${RTL433_SRC_DIR}/devices/radiohead_ask.c
    ${RTL433_SRC_DIR}/devices/rainpoint_hcs012arf.c
    ${RTL433_SRC_DIR}/devices/rainpoint.c
    ${RTL433_SRC_DIR}/devices/regency_fan.c
    ${RTL433_SRC_DIR}/devices/revolt_nc5462.c
    ${RTL433_SRC_DIR}/devices/revolt_zx7717.c
    ${RTL433_SRC_DIR}/devices/rftech.c
    ${RTL433_SRC_DIR}/devices/risco_agility.c
    ${RTL433_SRC_DIR}/devices/rojaflex.c
    ${RTL433_SRC_DIR}/devices/rosstech_dcu706.c
    ${RTL433_SRC_DIR}/devices/rubicson_48659.c
    ${RTL433_SRC_DIR}/devices/rubicson_pool_48942.c
    ${RTL433_SRC_DIR}/devices/rubicson.c
    ${RTL433_SRC_DIR}/devices/s3318p.c
    ${RTL433_SRC_DIR}/devices/sainlogic_sa8.c
    ${RTL433_SRC_DIR}/devices/schou_72543_rain.c
    ${RTL433_SRC_DIR}/devices/schraeder.c
    ${RTL433_SRC_DIR}/devices/scmplus.c
    ${RTL433_SRC_DIR}/devices/secplus_v1.c
    ${RTL433_SRC_DIR}/devices/secplus_v2.c
    ${RTL433_SRC_DIR}/devices/sharp_spc775.c
    ${RTL433_SRC_DIR}/devices/silvercrest.c
    ${RTL433_SRC_DIR}/devices/simplisafe_gen3.c
    ${RTL433_SRC_DIR}/devices/simplisafe.c
    ${RTL433_SRC_DIR}/devices/smoke_gs558.c
    ${RTL433_SRC_DIR}/devices/solight_te44.c
    ${RTL433_SRC_DIR}/devices/somfy_iohc.c
    ${RTL433_SRC_DIR}/devices/somfy_rts.c
    ${RTL433_SRC_DIR}/devices/springfield.c
    ${RTL433_SRC_DIR}/devices/srsmith_pool_srs_2c_tx.c
    ${RTL433_SRC_DIR}/devices/steelmate.c
    ${RTL433_SRC_DIR}/devices/telldus_ft0385r.c
    ${RTL433_SRC_DIR}/devices/tfa_14_1504_v2.c
    ${RTL433_SRC_DIR}/devices/tfa_30_3196.c
    ${RTL433_SRC_DIR}/devices/tfa_30_3221.c
    ${RTL433_SRC_DIR}/devices/tfa_drop_30.3233.c
    ${RTL433_SRC_DIR}/devices/tfa_marbella.c
    ${RTL433_SRC_DIR}/devices/tfa_pool_thermometer.c
    ${RTL433_SRC_DIR}/devices/tfa_twin_plus_30.3049.c
    ${RTL433_SRC_DIR}/devices/thermopro_tp11.c
    ${RTL433_SRC_DIR}/devices/thermopro_tp12.c
    ${RTL433_SRC_DIR}/devices/thermopro_tp211b.c
    ${RTL433_SRC_DIR}/devices/thermopro_tp28b.c
    ${RTL433_SRC_DIR}/devices/thermopro_tp82xb.c
    ${RTL433_SRC_DIR}/devices/thermopro_tp862b.c
    ${RTL433_SRC_DIR}/devices/thermopro_tx2.c
    ${RTL433_SRC_DIR}/devices/thermopro_tx2c.c
    ${RTL433_SRC_DIR}/devices/thermopro_tx7b.c
    ${RTL433_SRC_DIR}/devices/thermor.c
    ${RTL433_SRC_DIR}/devices/tpms_abarth124.c
    ${RTL433_SRC_DIR}/devices/tpms_airpuxem.c
    ${RTL433_SRC_DIR}/devices/tpms_ave.c
    ${RTL433_SRC_DIR}/devices/tpms_bmw_g3.c
    ${RTL433_SRC_DIR}/devices/tpms_bmw.c
    ${RTL433_SRC_DIR}/devices/tpms_citroen.c
    ${RTL433_SRC_DIR}/devices/tpms_eezrv.c
    ${RTL433_SRC_DIR}/devices/tpms_elantra2012.c
    ${RTL433_SRC_DIR}/devices/tpms_ford.c
    ${RTL433_SRC_DIR}/devices/tpms_gm.c
    ${RTL433_SRC_DIR}/devices/tpms_hyundai_vdo.c
    ${RTL433_SRC_DIR}/devices/tpms_jansite_solar.c
    ${RTL433_SRC_DIR}/devices/tpms_jansite.c
    ${RTL433_SRC_DIR}/devices/tpms_kia.c
    ${RTL433_SRC_DIR}/devices/tpms_nissan.c
    ${RTL433_SRC_DIR}/devices/tpms_pmv107j.c
    ${RTL433_SRC_DIR}/devices/tpms_porsche.c
    ${RTL433_SRC_DIR}/devices/tpms_renault_0435r.c
    ${RTL433_SRC_DIR}/devices/tpms_renault.c
    ${RTL433_SRC_DIR}/devices/tpms_toyota.c
    ${RTL433_SRC_DIR}/devices/tpms_truck.c
    ${RTL433_SRC_DIR}/devices/tpms_trw.c
    ${RTL433_SRC_DIR}/devices/tpms_tyreguard400.c
    ${RTL433_SRC_DIR}/devices/ts_ft002.c
    ${RTL433_SRC_DIR}/devices/ttx201.c
    ${RTL433_SRC_DIR}/devices/universalfanctrl.c
    ${RTL433_SRC_DIR}/devices/vaillant_vrt340f.c
    ${RTL433_SRC_DIR}/devices/vauno_en8822c.c
    ${RTL433_SRC_DIR}/devices/vevor_7in1.c
    ${RTL433_SRC_DIR}/devices/visonic_powercode.c
    ${RTL433_SRC_DIR}/devices/wallarge_cltx001.c
    ${RTL433_SRC_DIR}/devices/watts_thermostat.c
    ${RTL433_SRC_DIR}/devices/waveman.c
    ${RTL433_SRC_DIR}/devices/wec2103.c
    ${RTL433_SRC_DIR}/devices/wg_pb12v1.c
    ${RTL433_SRC_DIR}/devices/ws2032.c
    ${RTL433_SRC_DIR}/devices/wssensor.c
    ${RTL433_SRC_DIR}/devices/wt0124.c
    ${RTL433_SRC_DIR}/devices/wt450.c
    ${RTL433_SRC_DIR}/devices/x10_rf.c
    ${RTL433_SRC_DIR}/devices/x10_sec.c
    ${RTL433_SRC_DIR}/devices/yale_hsa.c
)

# rtl_433 build switches, scoped to the target rather than set with add_definitions():
# a directory-scope definition would also reach ebc-sdr-native.
#   RTLSDR, LIBUSB1, THREADS   rtl_433's own switches -- sdr.c compiles its RTL-SDR path and
#                              its libusb error names, and the decoder loop uses pthreads.
#                              Nothing in ebc-sdr-native tests them.
#   __EBCANDROID__             the Android entry point in rtl_433.c and the fd-open path in
#                              sdr.c are guarded by it. In ebc-sdr-native the same name is a
#                              comment convention rather than a compile guard, so it makes no
#                              difference there either way -- keeping it out is still correct.
target_compile_definitions(rtlsdr433 PRIVATE
    RTLSDR=1
    LIBUSB1=1
    THREADS=1
    __EBCANDROID__=1
)

find_library(log-lib log)

# Enable strict warnings for OUR bridge files only (not third-party rtl_433).
# ebc-sdr-native applies its own set, including -Werror=shorten-64-to-32, to its own files.
set_source_files_properties(
    rtlsdr433.cpp
    android_bridge.c
    PROPERTIES COMPILE_FLAGS "-Wall -Wextra -Wno-unused-parameter"
)

# ebc_sdr brings rtl-sdr, libusb, the bridge, their include paths, -fvisibility=hidden and the
# 16 KB page-alignment linker flags. It also links the NDK log library itself, but ${log-lib}
# stays listed here because this app's own files call __android_log_print directly.
target_link_libraries(rtlsdr433 PRIVATE ebc_sdr ${log-lib})
