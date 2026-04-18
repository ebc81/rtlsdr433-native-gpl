cmake_minimum_required(VERSION 3.22.1)

project("rtlsdr433")

# =============================================================================
# Native Module Layout
# =============================================================================
# This builds a single shared library 'librtlsdr433' composed of:
#
#   libusb-andro/  - libusb adapted for Android (no udev, uses linux_usbfs)
#   rtl-sdr/       - RTL-SDR hardware driver (tuner chips: R82XX, E4K, FC0012/13/2580)
#   rtl433/        - rtl_433 decoding engine (100+ 433 MHz device decoders)
#   rtlsdr433.cpp  - JNI bridge: exposes start/stop/status to Kotlin NativeBridge
#   librtlsdr_andro.c - Android-specific USB open via file descriptor (rtlsdr_open2)
#
# Output pipeline: rtl_433 decoder -> announce_output_line() [rtlsdr433.cpp]
#                  -> NativeBridge.nativeOutputLine() [JNI callback to Kotlin]
# =============================================================================

set(RTL433_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rtl433/src)
set(RTL_SDR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rtl-sdr)
set(LIBUSB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libusb-andro/libusb)

include_directories(
    ${RTL_SDR_DIR}/include
    ${LIBUSB_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/rtl433/include
    ${CMAKE_CURRENT_SOURCE_DIR}/rtl433
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Enable RTLSDR support and threading for Android
add_definitions(-DRTLSDR=1 -DLIBUSB1=1 -DTHREADS=1 -D__EBCANDROID__=1)

add_library(rtlsdr433 SHARED
    rtlsdr433.cpp
    librtlsdr_andro.c
    android_bridge.c

    # libusb sources
    ${LIBUSB_DIR}/core.c
    ${LIBUSB_DIR}/descriptor.c
    ${LIBUSB_DIR}/io.c
    ${LIBUSB_DIR}/sync.c
    ${LIBUSB_DIR}/strerror.c
    ${LIBUSB_DIR}/hotplug.c
    ${LIBUSB_DIR}/os/linux_usbfs.c
    ${LIBUSB_DIR}/os/threads_posix.c
    ${LIBUSB_DIR}/os/linux_netlink.c
    ${LIBUSB_DIR}/os/poll_posix.c

    # rtl-sdr sources
    ${RTL_SDR_DIR}/src/tuner_r82xx.c
    ${RTL_SDR_DIR}/src/tuner_e4k.c
    ${RTL_SDR_DIR}/src/tuner_fc0012.c
    ${RTL_SDR_DIR}/src/tuner_fc0013.c
    ${RTL_SDR_DIR}/src/tuner_fc2580.c

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

find_library(log-lib log)

# Enable strict warnings for OUR bridge files only (not third-party rtl_433/libusb/rtl-sdr)
set_source_files_properties(
    rtlsdr433.cpp
    android_bridge.c
    librtlsdr_andro.c
    PROPERTIES COMPILE_FLAGS "-Wall -Wextra -Wno-unused-parameter"
)

target_link_libraries(rtlsdr433 ${log-lib})
