#ifndef __CV_OUT_H__
#define __CV_OUT_H__

void init_cv_out();
void update_CV_outs();
void update_CV_outs_manual_calibration();
void write_cv_pwm();
void write_cv_pwm_manual_calibration(uint8_t voice, uint16_t vca);

void cv_bake_adsr2_to_vcf_scale();
void cv_bake_lfo2_to_vcf_scale();
void cv_bake_lfo1_to_vca_scale();
void cv_update_mod_scales();

#endif
