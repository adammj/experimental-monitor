################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
common/internal/cla.obj: /Users/adam/Documents/Projects/Experimental\ Monitor/em_c_code/common/internal/cla.cpp $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla1 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu2 -O4 --opt_for_speed=3 --fp_mode=relaxed --fp_reassoc=off --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/EM_CPU1" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/experiment/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/internal/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/analog_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/digital_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/dsp_output/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/serial" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/support" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/ti_launchpad/79D_ti_headers" --include_path="/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/include" --advice:performance=all --define=_DEBUG --define=CPU1 --define=_LAUNCHXL_F28379D -g --symdebug:dwarf_version=3 --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --issue_remarks --verbose_diagnostics --abi=coffabi --cla_signed_compare_workaround=off --preproc_with_compile --preproc_dependency="common/internal/cla.d_raw" --obj_directory="common/internal" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

common/internal/cpu_timers.obj: /Users/adam/Documents/Projects/Experimental\ Monitor/em_c_code/common/internal/cpu_timers.cpp $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla1 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu2 -O4 --opt_for_speed=3 --fp_mode=relaxed --fp_reassoc=off --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/EM_CPU1" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/experiment/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/internal/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/analog_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/digital_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/dsp_output/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/serial" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/support" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/ti_launchpad/79D_ti_headers" --include_path="/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/include" --advice:performance=all --define=_DEBUG --define=CPU1 --define=_LAUNCHXL_F28379D -g --symdebug:dwarf_version=3 --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --issue_remarks --verbose_diagnostics --abi=coffabi --cla_signed_compare_workaround=off --preproc_with_compile --preproc_dependency="common/internal/cpu_timers.d_raw" --obj_directory="common/internal" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

common/internal/leds.obj: /Users/adam/Documents/Projects/Experimental\ Monitor/em_c_code/common/internal/leds.cpp $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla1 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu2 -O4 --opt_for_speed=3 --fp_mode=relaxed --fp_reassoc=off --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/EM_CPU1" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/experiment/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/internal/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/analog_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/digital_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/dsp_output/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/serial" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/support" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/ti_launchpad/79D_ti_headers" --include_path="/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/include" --advice:performance=all --define=_DEBUG --define=CPU1 --define=_LAUNCHXL_F28379D -g --symdebug:dwarf_version=3 --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --issue_remarks --verbose_diagnostics --abi=coffabi --cla_signed_compare_workaround=off --preproc_with_compile --preproc_dependency="common/internal/leds.d_raw" --obj_directory="common/internal" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

common/internal/sci.obj: /Users/adam/Documents/Projects/Experimental\ Monitor/em_c_code/common/internal/sci.cpp $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla1 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu2 -O4 --opt_for_speed=3 --fp_mode=relaxed --fp_reassoc=off --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/EM_CPU1" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/experiment/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/internal/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/analog_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/digital_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/dsp_output/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/serial" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/support" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/ti_launchpad/79D_ti_headers" --include_path="/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/include" --advice:performance=all --define=_DEBUG --define=CPU1 --define=_LAUNCHXL_F28379D -g --symdebug:dwarf_version=3 --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --issue_remarks --verbose_diagnostics --abi=coffabi --cla_signed_compare_workaround=off --preproc_with_compile --preproc_dependency="common/internal/sci.d_raw" --obj_directory="common/internal" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

common/internal/scia.obj: /Users/adam/Documents/Projects/Experimental\ Monitor/em_c_code/common/internal/scia.cpp $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla1 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu2 -O4 --opt_for_speed=3 --fp_mode=relaxed --fp_reassoc=off --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/EM_CPU1" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/experiment/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/internal/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/analog_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/digital_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/dsp_output/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/serial" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/support" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/ti_launchpad/79D_ti_headers" --include_path="/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/include" --advice:performance=all --define=_DEBUG --define=CPU1 --define=_LAUNCHXL_F28379D -g --symdebug:dwarf_version=3 --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --issue_remarks --verbose_diagnostics --abi=coffabi --cla_signed_compare_workaround=off --preproc_with_compile --preproc_dependency="common/internal/scia.d_raw" --obj_directory="common/internal" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

common/internal/spi.obj: /Users/adam/Documents/Projects/Experimental\ Monitor/em_c_code/common/internal/spi.cpp $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla1 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu2 -O4 --opt_for_speed=3 --fp_mode=relaxed --fp_reassoc=off --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/EM_CPU1" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/experiment/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/internal/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/analog_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/digital_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/dsp_output/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/serial" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/support" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/ti_launchpad/79D_ti_headers" --include_path="/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/include" --advice:performance=all --define=_DEBUG --define=CPU1 --define=_LAUNCHXL_F28379D -g --symdebug:dwarf_version=3 --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --issue_remarks --verbose_diagnostics --abi=coffabi --cla_signed_compare_workaround=off --preproc_with_compile --preproc_dependency="common/internal/spi.d_raw" --obj_directory="common/internal" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

common/internal/status_messages.obj: /Users/adam/Documents/Projects/Experimental\ Monitor/em_c_code/common/internal/status_messages.cpp $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla1 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu2 -O4 --opt_for_speed=3 --fp_mode=relaxed --fp_reassoc=off --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/EM_CPU1" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/experiment/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/internal/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/analog_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/digital_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/dsp_output/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/serial" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/support" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/ti_launchpad/79D_ti_headers" --include_path="/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/include" --advice:performance=all --define=_DEBUG --define=CPU1 --define=_LAUNCHXL_F28379D -g --symdebug:dwarf_version=3 --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --issue_remarks --verbose_diagnostics --abi=coffabi --cla_signed_compare_workaround=off --preproc_with_compile --preproc_dependency="common/internal/status_messages.d_raw" --obj_directory="common/internal" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

common/internal/tic_toc.obj: /Users/adam/Documents/Projects/Experimental\ Monitor/em_c_code/common/internal/tic_toc.cpp $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla1 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu2 -O4 --opt_for_speed=3 --fp_mode=relaxed --fp_reassoc=off --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/EM_CPU1" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/experiment/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/internal/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/analog_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/digital_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/dsp_output/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/serial" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/support" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/ti_launchpad/79D_ti_headers" --include_path="/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/include" --advice:performance=all --define=_DEBUG --define=CPU1 --define=_LAUNCHXL_F28379D -g --symdebug:dwarf_version=3 --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --issue_remarks --verbose_diagnostics --abi=coffabi --cla_signed_compare_workaround=off --preproc_with_compile --preproc_dependency="common/internal/tic_toc.d_raw" --obj_directory="common/internal" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

common/internal/tic_toc_cla.obj: /Users/adam/Documents/Projects/Experimental\ Monitor/em_c_code/common/internal/tic_toc_cla.cla $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla1 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu2 -O4 --opt_for_speed=3 --fp_mode=relaxed --fp_reassoc=off --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/EM_CPU1" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/experiment/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/internal/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/analog_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/digital_io/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/dsp_output/" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/serial" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/common/support" --include_path="/Users/adam/Documents/Projects/Experimental Monitor/em_c_code/ti_launchpad/79D_ti_headers" --include_path="/Applications/ti/ccs1010/ccs/tools/compiler/ti-cgt-c2000_20.2.1.LTS/include" --advice:performance=all --define=_DEBUG --define=CPU1 --define=_LAUNCHXL_F28379D -g --symdebug:dwarf_version=3 --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --issue_remarks --verbose_diagnostics --abi=coffabi --cla_signed_compare_workaround=off --preproc_with_compile --preproc_dependency="common/internal/tic_toc_cla.d_raw" --obj_directory="common/internal" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


