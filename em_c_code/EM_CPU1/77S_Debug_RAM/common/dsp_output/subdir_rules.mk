################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
common/dsp_output/dsp_output.obj: /Users/adam/Documents/Projects/Experimental\ Monitor/em_c_code/common/dsp_output/dsp_output.cpp $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla1 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu2 -O4 --opt_for_speed=2 --fp_mode=relaxed --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/dsp_output/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/EM_CPU1" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/experiment/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/analog_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/digital_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/internal/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/serial" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/support" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/ti_launchpad/77S_ti_headers" --include_path="/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/include" --advice:performance=all --define=_DEBUG --define=CPU1 --define=_LAUNCHXL_F28377S -g --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --verbose_diagnostics --abi=coffabi --cla_signed_compare_workaround=off --preproc_with_compile --preproc_dependency="common/dsp_output/dsp_output.d_raw" --obj_directory="common/dsp_output" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


