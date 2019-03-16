#!/bin/sh

#######################################################################
# Hardware Audio init
#######################################################################
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
amixer cset -c 1 name="x Speaker Playback Volume" 100               ## reg-01: 8080
############################################################################



# in1n init
############################################################################
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
amixer cset -c tegrasndt186ref name="x ADC Capture Volume" 50 50
amixer cset -c tegrasndt186ref name="x RECMIXR BST2 Switch" 1
amixer cset -c tegrasndt186ref name="x RECMIXR BST1 Switch" 0
amixer cset -c tegrasndt186ref name="x RECMIXL BST2 Switch" 1
amixer cset -c tegrasndt186ref name="x RECMIXL BST1 Switch" 0
amixer sset -c tegrasndt186ref "ADMAIF1 Mux" "I2S1"
amixer sset -c tegrasndt186ref "I2S1 Mux" "ADMAIF1"
amixer cset -c tegrasndt186ref name='x ADC Boost Gain'   1

# in1p init
############################################################################
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
amixer cset -c tegrasndt186ref name='x ADC Boost Gain'   1

