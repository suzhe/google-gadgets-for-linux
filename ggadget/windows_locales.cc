/*
  Copyright 2007 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <algorithm>
#include <cstdio>
#include <cstring>
#include "common.h"
#include "windows_locales.h"

namespace ggadget {

// Stores locale names and Windows locale identifiers.
// Identifiers are stored as shorts.
// They're converted to strings as needed.
struct LocaleNameAndId {
  const char *name;
  short id;
};

// The locale names to Windows locale identifiers mapping table.
// Source for listing is:
//   http://msdn2.microsoft.com/en-us/library/ms776260.aspx
// List below is sorted in ALPHABETICAL ORDER by locale name for use in
// binary search.
static const LocaleNameAndId kLocaleNames[] = {
  {"af_ZA", 0x0436},
  {"am_ET", 0x045e},
  {"ar_AE", 0x3801},
  {"ar_BH", 0x3c01},
  {"ar_DZ", 0x1401},
  {"ar_EG", 0x0c01},
  {"ar_IQ", 0x0801},
  {"ar_JO", 0x2c01},
  {"ar_KW", 0x3401},
  {"ar_LB", 0x3001},
  {"ar_LY", 0x1001},
  {"ar_MA", 0x1801},
  {"arn_CL", 0x047a},
  {"ar_OM", 0x2001},
  {"ar_QA", 0x4001},
  {"ar_SA", 0x0401},
  {"ar_SY", 0x2801},
  {"ar_TN", 0x1c01},
  {"ar_YE", 0x2401},
  {"as_IN", 0x044d},
  {"az_Cyrl_AZ", 0x082c},
  {"az_Latn_AZ", 0x042c},
  {"ba_RU", 0x046d},
  {"be_BY", 0x0423},
  {"bg_BG", 0x0402},
  {"bn_IN", 0x0445},
  {"bo_BT", 0x0851},
  {"bo_CN", 0x0451},
  {"br_FR", 0x047e},
  {"bs_Cyrl_BA", 0x201a},
  {"bs_Latn_BA", 0x141a},
  {"ca_ES", 0x0403},
  {"cs_CZ", 0x0405},
  {"cy_GB", 0x0452},
  {"da_DK", 0x0406},
  {"de_AT", 0x0c07},
  {"de_CH", 0x0807},
  {"de_DE", 0x0407},
  {"de_LI", 0x1407},
  {"de_LU", 0x1007},
  {"dsb_DE", 0x082e},
  {"dv_MV", 0x0465},
  {"el_GR", 0x0408},
  {"en_029", 0x2409},
  {"en_AU", 0x0c09},
  {"en_BZ", 0x2809},
  {"en_CA", 0x1009},
  {"en_GB", 0x0809},
  {"en_IE", 0x1809},
  {"en_IN", 0x4009},
  {"en_JM", 0x2009},
  {"en_MY", 0x4409},
  {"en_NZ", 0x1409},
  {"en_PH", 0x3409},
  {"en_SG", 0x4809},
  {"en_TT", 0x2c09},
  {"en_US", 0x0409},
  {"en_ZA", 0x1c09},
  {"en_ZW", 0x3009},
  {"es_AR", 0x2c0a},
  {"es_BO", 0x400a},
  {"es_CL", 0x340a},
  {"es_CO", 0x240a},
  {"es_CR", 0x140a},
  {"es_DO", 0x1c0a},
  {"es_EC", 0x300a},
  {"es_ES", 0x0c0a},
  {"es_ES_tradnl", 0x040a},
  {"es_GT", 0x100a},
  {"es_HN", 0x480a},
  {"es_MX", 0x080a},
  {"es_NI", 0x4c0a},
  {"es_PA", 0x180a},
  {"es_PE", 0x280a},
  {"es_PR", 0x500a},
  {"es_PY", 0x3c0a},
  {"es_SV", 0x440a},
  {"es_US", 0x540a},
  {"es_UY", 0x380a},
  {"es_VE", 0x200a},
  {"et_EE", 0x0425},
  {"eu_ES", 0x042d},
  {"fa_IR", 0x0429},
  {"fi_FI", 0x040b},
  {"fil_PH", 0x0464},
  {"fo_FO", 0x0438},
  {"fr_BE", 0x080c},
  {"fr_CA", 0x0c0c},
  {"fr_CH", 0x100c},
  {"fr_FR", 0x040c},
  {"fr_LU", 0x140c},
  {"fr_MC", 0x180c},
  {"fy_NL", 0x0462},
  {"ga_IE", 0x083c},
  {"gbz_AF", 0x048c},
  {"gl_ES", 0x0456},
  {"gsw_FR", 0x0484},
  {"gu_IN", 0x0447},
  {"ha_Latn_NG", 0x0468},
  {"he_IL", 0x040d},
  {"hi_IN", 0x0439},
  {"hr_BA", 0x101a},
  {"hr_HR", 0x041a},
  {"hu_HU", 0x040e},
  {"hy_AM", 0x042b},
  {"id_ID", 0x0421},
  {"ig_NG", 0x0470},
  {"ii_CN", 0x0478},
  {"is_IS", 0x040f},
  {"it_CH", 0x0810},
  {"it_IT", 0x0410},
  {"iu_Cans_CA", 0x045d},
  {"iu_Latn_CA", 0x085d},
  {"ja_JP", 0x0411},
  {"ka_GE", 0x0437},
  {"kh_KH", 0x0453},
  {"kk_KZ", 0x043f},
  {"kl_GL", 0x046f},
  {"kn_IN", 0x044b},
  {"kok_IN", 0x0457},
  {"ko_KR", 0x0412},
  {"ky_KG", 0x0440},
  {"lb_LU", 0x046e},
  {"lo_LA", 0x0454},
  {"lt_LT", 0x0427},
  {"lv_LV", 0x0426},
  {"mi_NZ", 0x0481},
  {"mk_MK", 0x042f},
  {"ml_IN", 0x044c},
  {"mn_Cyrl_MN", 0x0450},
  {"mn_Mong_CN", 0x0850},
  {"moh_CA", 0x047c},
  {"mr_IN", 0x044e},
  {"ms_BN", 0x083e},
  {"ms_MY", 0x043e},
  {"mt_MT", 0x043a},
  {"nb_NO", 0x0414},
  {"ne_NP", 0x0461},
  {"nl_BE", 0x0813},
  {"nl_NL", 0x0413},
  {"nn_NO", 0x0814},
  {"ns_ZA", 0x046c},
  {"oc_FR", 0x0482},
  {"or_IN", 0x0448},
  {"pa_IN", 0x0446},
  {"pl_PL", 0x0415},
  {"ps_AF", 0x0463},
  {"pt_BR", 0x0416},
  {"pt_PT", 0x0816},
  {"qut_GT", 0x0486},
  {"quz_BO", 0x046b},
  {"quz_EC", 0x086b},
  {"quz_PE", 0x0c6b},
  {"rm_CH", 0x0417},
  {"ro_RO", 0x0418},
  {"ru_RU", 0x0419},
  {"rw_RW", 0x0487},
  {"sah_RU", 0x0485},
  {"sa_IN", 0x044f},
  {"se_FI", 0x0c3b},
  {"se_NO", 0x043b},
  {"se_SE", 0x083b},
  {"si_LK", 0x045b},
  {"sk_SK", 0x041b},
  {"sl_SI", 0x0424},
  {"sma_NO", 0x183b},
  {"sma_SE", 0x1c3b},
  {"smj_NO", 0x103b},
  {"smj_SE", 0x143b},
  {"smn_FI", 0x243b},
  {"sms_FI", 0x203b},
  {"sq_AL", 0x041c},
  {"sr_Cyrl_BA", 0x1c1a},
  {"sr_Cyrl_CS", 0x0c1a},
  {"sr_Latn_BA", 0x181a},
  {"sr_Latn_CS", 0x081a},
  {"sv_FI", 0x081d},
  {"sv_SE", 0x041d},
  {"sw_KE", 0x0441},
  {"syr_SY", 0x045a},
  {"ta_IN", 0x0449},
  {"te_IN", 0x044a},
  {"tg_Cyrl_TJ", 0x0428},
  {"th_TH", 0x041e},
  {"tk_TM", 0x0442},
  {"tmz_Latn_DZ", 0x085f},
  {"tn_ZA", 0x0432},
  {"tr_IN", 0x0820},
  {"tr_TR", 0x041f},
  {"tt_RU", 0x0444},
  {"ug_CN", 0x0480},
  {"uk_UA", 0x0422},
  {"ur_PK", 0x0420},
  {"uz_Cyrl_UZ", 0x0843},
  {"uz_Latn_UZ", 0x0443},
  {"vi_VN", 0x042a},
  {"wen_DE", 0x042e},
  {"wo_SN", 0x0488},
  {"xh_ZA", 0x0434},
  {"yo_NG", 0x046a},
  {"zh_CN", 0x0804},
  {"zh_HK", 0x0c04},
  {"zh_MO", 0x1404},
  {"zh_SG", 0x1004},
  {"zh_TW", 0x0404},
  {"zu_ZA", 0x0435}
};

static inline bool LocaleNameAndIdCompare(const LocaleNameAndId &v1,
                                          const LocaleNameAndId &v2) {
  return strcmp(v1.name, v2.name) < 0;
}

// Returns true if ID found.
bool GetLocaleIDString(const char *name, std::string *id) {
  ASSERT(id);
  LocaleNameAndId key = { name, 0 };
  const LocaleNameAndId *pos = std::lower_bound(
      kLocaleNames, kLocaleNames + arraysize(kLocaleNames), key,
      LocaleNameAndIdCompare);

  ASSERT(pos);
  if (strcmp(name, pos->name) == 0) {
    char buffer[12];
    int identifier = pos->id;
    snprintf(buffer, sizeof(buffer), "%d", identifier);
    *id = std::string(buffer);
    return true;
  }

  return false;
}

} // namespace ggadget
