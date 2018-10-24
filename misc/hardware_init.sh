#!/bin/bash


######################################################################
#
# Disabled NTP time (07-24-2018)
#
######################################################################
timedatectl set-ntp false



######################################################################
# USB HUB & Sensor Module init
# DVT: Used gpio 457, 461
#
######################################################################
i2cset -f -y 0 0x77 0x2 0x07
i2cset -f -y 0 0x77 0x3 0x70
i2cset -f -y 0 0x77 0x6 0x00
i2cset -f -y 0 0x77 0x7 0x00



#echo 336 > /sys/class/gpio/export
#echo 390 > /sys/class/gpio/export
echo 457 > /sys/class/gpio/export
echo 461 > /sys/class/gpio/export

echo out > /sys/class/gpio/gpio457/direction
#echo out > /sys/class/gpio/gpio336/direction
sleep 0.5
#echo out > /sys/class/gpio/gpio390/direction
echo out > /sys/class/gpio/gpio461/direction



#######################################################################
# >>>> Hardware Audio init
#######################################################################
# playback

# For Playback:
amixer cset -c tegrasndt186ref name="I2S1 Mux" 20
amixer cset -c tegrasndt186ref name="MIXER1-1 Mux" 1
amixer cset -c tegrasndt186ref name="Adder1 RX1" 1
amixer cset -c tegrasndt186ref name="Mixer Enable" 1
amixer cset -c tegrasndt186ref name="ADMAIF1 Mux" 11
amixer cset -c tegrasndt186ref name="x Int Spk Switch" 1
amixer cset -c tegrasndt186ref name="x Speaker Playback Volume" 100
amixer cset -c tegrasndt186ref name="x Headphone Jack Switch" 0
amixer sset -c tegrasndt186ref 'MIXER1-1 Mux' 'ADMAIF1'
amixer sset -c tegrasndt186ref 'I2S1 Mux' 'MIXER1-1'
amixer cset -c 1 name="x Speaker Playback Volume" 100  ## reg-01: 8080




#in1n init

    # reg-28: 5050 -> 1010
    amixer cset -c tegrasndt186ref name="x Mono ADC MIXR ADC1 Switch" 1
    amixer cset -c tegrasndt186ref name="x Mono ADC MIXL ADC1 Switch" 1
    # reg-3c: 00x4f -> 006f
    amixer cset -c tegrasndt186ref name="x RECMIXR INR Switch" 1
    # reg-3e: 004f -> 00x6f
    amixer cset -c tegrasndt186ref name="x RECMIXL INL Switch" 1

    amixer cset -c tegrasndt186ref name="x DMIC Switch" "DMIC1"
    amixer cset -c tegrasndt186ref name="x Stereo ADC R2 Mux" "DMIC1"
    amixer cset -c tegrasndt186ref name="x Stereo ADC L2 Mux" "DMIC1"
    amixer cset -c tegrasndt186ref name="x Mono ADC R2 Mux" "DMIC R1"
    amixer cset -c tegrasndt186ref name="x Mono ADC L2 Mux" "DMIC L1"
    amixer cset -c tegrasndt186ref name="x Stereo ADC MIXR ADC1 Switch" 1
    amixer cset -c tegrasndt186ref name="x Stereo ADC MIXL ADC1 Switch" 1
    amixer cset -c tegrasndt186ref name="x Stereo ADC MIXR ADC2 Switch" 1
    amixer cset -c tegrasndt186ref name="x Stereo ADC MIXL ADC2 Switch" 1

    amixer cset -c tegrasndt186ref name="x Mono ADC MIXL ADC2 Switch" 1
    amixer cset -c tegrasndt186ref name="x Mono ADC MIXR ADC2 Switch" 1

    amixer cset -c tegrasndt186ref name="x IN1 Mode Control" 0
    amixer cset -c tegrasndt186ref name="x IN1 Boost" 2
    amixer cset -c tegrasndt186ref name="x ADC Capture Volume" 127 127
    amixer cset -c tegrasndt186ref name="x RECMIXR BST2 Switch" 1
    amixer cset -c tegrasndt186ref name="x RECMIXR BST1 Switch" 0
    amixer cset -c tegrasndt186ref name="x RECMIXL BST2 Switch" 1
    amixer cset -c tegrasndt186ref name="x RECMIXL BST1 Switch" 0

    # make it work
    amixer cset -c tegrasndt186ref name="x IF2 ADC R Mux" "Mono ADC MIXR"
    amixer cset -c tegrasndt186ref name="x IF2 ADC L Mux" "Mono ADC MIXL"
    amixer cset -c tegrasndt186ref name="x Mono ADC MIXR ADC1 Switch" 1
    amixer cset -c tegrasndt186ref name="x Mono ADC MIXL ADC1 Switch" 1
    amixer sset -c tegrasndt186ref "ADMAIF2 Mux" "I2S2"
    amixer sset -c tegrasndt186ref "I2S2 Mux" "ADMAIF2"
    amixer cset -c tegrasndt186ref name='x ADC Boost Gain'   2

#in1p init
    amixer cset -c tegrasndt186ref name="x DMIC Switch" "DMIC2"
    amixer cset -c tegrasndt186ref name="x Stereo ADC R2 Mux" "DMIC2"
    #amixer cset -c tegrasndt186ref name="x Stereo ADC L2 Mux" "DMIC2"
    amixer cset -c tegrasndt186ref name="x Mono ADC R2 Mux" "DMIC R1"
    #amixer cset -c tegrasndt186ref name="x Mono ADC L2 Mux" "DMIC L2"
    amixer cset -c tegrasndt186ref name="x Stereo ADC MIXR ADC1 Switch" 1
    amixer cset -c tegrasndt186ref name="x Stereo ADC MIXL ADC1 Switch" 1
    amixer cset -c tegrasndt186ref name="x Stereo ADC MIXR ADC2 Switch" 1
    amixer cset -c tegrasndt186ref name="x Stereo ADC MIXL ADC2 Switch" 1

    amixer cset -c tegrasndt186ref name="x Mono ADC MIXL ADC2 Switch" 1
    amixer cset -c tegrasndt186ref name="x Mono ADC MIXR ADC2 Switch" 1

    # amixer cset -c tegrasndt186ref name="x IN1 Mode Control" 0
    amixer cset -c tegrasndt186ref name="x IN1 Boost" 1
    amixer cset -c tegrasndt186ref name="x ADC Capture Volume" 57 57
    amixer cset -c tegrasndt186ref name="x RECMIXR BST2 Switch" 1
    amixer cset -c tegrasndt186ref name="x RECMIXR BST1 Switch" 0
    amixer cset -c tegrasndt186ref name="x RECMIXL BST2 Switch" 1
    amixer cset -c tegrasndt186ref name="x RECMIXL BST1 Switch" 0
    amixer sset -c tegrasndt186ref "ADMAIF1 Mux" "I2S1"
    amixer sset -c tegrasndt186ref "I2S1 Mux" "ADMAIF1"

    amixer cset -c tegrasndt186ref name="x IN2 Mode Control" 0
    amixer cset -c tegrasndt186ref name="x IN2 Boost" 1
    amixer sset -c tegrasndt186ref "ADMAIF1 Mux" "I2S1"
    amixer sset -c tegrasndt186ref "I2S1 Mux" "ADMAIF1"


################################################################################
# GPS Init
# DVT: Used gpio303 for GPS Reset
# PVT: H3(GPIO3_PJ.05, 397) for GPS Power
################################################################################
echo 397 > /sys/class/gpio/export
echo 303 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio303/direction
echo out > /sys/class/gpio/gpio397/direction
echo 1 > /sys/class/gpio/gpio397/value
sleep 0.5
echo 1 > sys/class/gpio/gpio303/value
sleep 0.1
echo 0 > /sys/class/gpio/gpio303/value






