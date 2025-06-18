/// does processing a long string with Cscope fail?
/// https://github.com/Smattr/clink/issues/279

int main(void) {

  const char big[] = "\
  digraph G {\n\
    rankdir=BT;\n\
    ranksep=0.7;\n\
    nodesep=0.3;\n\
    subgraph cluster_0 {\n\
    margin=0.5;\n\
    rankdir=BT;\n\
    ranksep=0.7;\n\
    nodesep=0.3;\n\
    \"cluster_0_start\" [ shape=box, width=\"3.157056384616428\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Start__Sawing_Machine\" ];\n\
  \"cluster_0_end\" [ shape=box, width=\"3.067291683620877\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"End__Sawing_Machine\" ];\n\
  \"cluster_0_8\" [ shape=box, width=\"2.4715493520100913\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"SM_Postprocessing\" ];\n\
  \"cluster_0_10\" [ shape=box, width=\"2.0178529951307507\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"SM_Producing\" ];\n\
  \"cluster_0_13\" [ shape=box, width=\"2.2250813378228083\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Sawing_Machine\" ];\n\
  \"cluster_0_2\" [ shape=box, width=\"1.6418668958875868\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"SM_Break\" ];\n\
  \"cluster_0_4\" [ shape=box, width=\"2.0078504350450306\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"SM_Microstop\" ];\n\
  \"cluster_0_1\" [ shape=box, width=\"2.125057644314236\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Production_End\" ];\n\
  \"cluster_0_6\" [ shape=box, width=\"3.0203959147135415\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"SM_Other_technical_issue\" ];\n\
  \"cluster_0_3\" [ shape=box, width=\"2.854460186428494\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"SM_Crane_not_available\" ];\n\
  \"cluster_0_12\" [ shape=box, width=\"2.4902716742621527\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"SM_Unknown_stop1\" ];\n\
  \"cluster_0_5\" [ shape=box, width=\"2.09171634250217\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"SM_Microstop1\" ];\n\
  \"cluster_0_11\" [ shape=box, width=\"2.406405766805013\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"SM_Unknown_stop\" ];\n\
  \"cluster_0_9\" [ shape=box, width=\"2.141728295220269\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"SM_Preparation\" ];\n\
  \"cluster_0_7\" [ shape=box, width=\"3.1042618221706815\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"SM_Other_technical_issue1\" ];\n\
    \"cluster_0_8\" -> \"cluster_0_end\" [ weight=\"1\", label=\"__31_5K\", width=\"0.7690972222222222\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_start\" -> \"cluster_0_13\" [ weight=\"1\", label=\"__61_6K\", width=\"0.7677951388888888\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_13\" -> \"cluster_0_10\" [ weight=\"30258\", label=\"__30_3K\", width=\"0.7986111111111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_10\" -> \"cluster_0_8\" [ weight=\"12561\", label=\"__12_6K\", width=\"0.771484375\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_start\" -> \"cluster_0_8\" [ weight=\"1\", label=\"__218\", width=\"0.6325954861111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_start\" -> \"cluster_0_10\" [ weight=\"1\", label=\"__3_97K\", width=\"0.7888454861111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_13\" -> \"cluster_0_end\" [ weight=\"1\", label=\"__26_7K\", width=\"0.7842881944444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_8\" -> \"cluster_0_13\" [ weight=\"218\", label=\"__218\", width=\"0.6325954861111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_13\" -> \"cluster_0_8\" [ weight=\"1710\", label=\"__1_71K\", width=\"0.7371961805555556\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_10\" -> \"cluster_0_13\" [ weight=\"2474\", label=\"__2_47K\", width=\"0.7944878472222222\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_10\" -> \"cluster_0_2\" [ weight=\"19188\", label=\"__19_2K\", width=\"0.771484375\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_2\" -> \"cluster_0_8\" [ weight=\"11569\", label=\"__11_6K\", width=\"0.7434895833333334\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_2\" -> \"cluster_0_end\" [ weight=\"1\", label=\"__1_49K\", width=\"0.7745225694444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_4\" -> \"cluster_0_8\" [ weight=\"8856\", label=\"__8_86K\", width=\"0.7934027777777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_2\" -> \"cluster_0_4\" [ weight=\"12546\", label=\"__12_5K\", width=\"0.7708333333333334\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_4\" -> \"cluster_0_4\" [ weight=\"3690\", label=\"__3_69K\", width=\"0.7970920138888888\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_1\" -> \"cluster_0_end\" [ weight=\"1\", label=\"__11_9K\", width=\"0.7463107638888888\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_13\" -> \"cluster_0_1\" [ weight=\"11932\", label=\"__11_9K\", width=\"0.7463107638888888\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_4\" -> \"cluster_0_6\" [ weight=\"3690\", label=\"__3_69K\", width=\"0.7970920138888888\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_6\" -> \"cluster_0_2\" [ weight=\"3690\", label=\"__3_69K\", width=\"0.7970920138888888\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_start\" -> \"cluster_0_6\" [ weight=\"1\", label=\"__2_37K\", width=\"0.7894965277777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_6\" -> \"cluster_0_13\" [ weight=\"2365\", label=\"__2_37K\", width=\"0.7894965277777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_4\" -> \"cluster_0_3\" [ weight=\"3321\", label=\"__3_32K\", width=\"0.7960069444444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_3\" -> \"cluster_0_4\" [ weight=\"3321\", label=\"__3_32K\", width=\"0.7960069444444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_start\" -> \"cluster_0_3\" [ weight=\"1\", label=\"__1_06K\", width=\"0.7680121527777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_3\" -> \"cluster_0_end\" [ weight=\"1\", label=\"__1_06K\", width=\"0.7680121527777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_3\" -> \"cluster_0_8\" [ weight=\"1491\", label=\"__1_49K\", width=\"0.7745225694444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_8\" -> \"cluster_0_3\" [ weight=\"1491\", label=\"__1_49K\", width=\"0.7745225694444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_start\" -> \"cluster_0_12\" [ weight=\"1\", label=\"__3_02K\", width=\"0.7944878472222222\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_12\" -> \"cluster_0_13\" [ weight=\"2328\", label=\"__2_33K\", width=\"0.7994791666666666\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_12\" -> \"cluster_0_end\" [ weight=\"1\", label=\"__1_71K\", width=\"0.7371961805555556\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_8\" -> \"cluster_0_12\" [ weight=\"1710\", label=\"__1_71K\", width=\"0.7371961805555556\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_start\" -> \"cluster_0_5\" [ weight=\"1\", label=\"__2_84K\", width=\"0.7979600694444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_5\" -> \"cluster_0_13\" [ weight=\"1601\", label=\"__1_6K\", width=\"0.6725260416666666\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_5\" -> \"cluster_0_5\" [ weight=\"1601\", label=\"__1_6K\", width=\"0.6725260416666666\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_11\" -> \"cluster_0_end\" [ weight=\"1\", label=\"__1_31K\", width=\"0.7452256944444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_13\" -> \"cluster_0_11\" [ weight=\"1310\", label=\"__1_31K\", width=\"0.7452256944444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_start\" -> \"cluster_0_11\" [ weight=\"1\", label=\"__691\", width=\"0.6365017361111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_12\" -> \"cluster_0_11\" [ weight=\"691\", label=\"__691\", width=\"0.6365017361111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_8\" -> \"cluster_0_9\" [ weight=\"1491\", label=\"__1_49K\", width=\"0.7745225694444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_9\" -> \"cluster_0_2\" [ weight=\"1491\", label=\"__1_49K\", width=\"0.7745225694444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_5\" -> \"cluster_0_7\" [ weight=\"1237\", label=\"__1_24K\", width=\"0.7693142361111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_7\" -> \"cluster_0_2\" [ weight=\"1237\", label=\"__1_24K\", width=\"0.7693142361111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  }\n\
  subgraph cluster_1 {\n\
    margin=0.5;\n\
    rankdir=BT;\n\
    ranksep=0.7;\n\
    nodesep=0.3;\n\
    \"cluster_1_start\" [ shape=box, width=\"3.3778775533040366\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Start__Pre_Assembly_Area\" ];\n\
  \"cluster_1_end\" [ shape=box, width=\"3.288112852308485\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"End__Pre_Assembly_Area\" ];\n\
  \"cluster_1_1\" [ shape=box, width=\"2.2096930609809027\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"E_Box_Assembly\" ];\n\
  \"cluster_1_2\" [ shape=box, width=\"1.990923563639323\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"E_box_Testing\" ];\n\
  \"cluster_1_3\" [ shape=box, width=\"2.7564885881212025\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Valve_Group_Assembly\" ];\n\
  \"cluster_1_4\" [ shape=box, width=\"2.67185295952691\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Valve_Plate_Assembly\" ];\n\
  \"cluster_1_5\" [ shape=box, width=\"3.1301663716634116\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Wire_Cutting_and_Crimping\" ];\n\
  \"cluster_1_6\" [ shape=box, width=\"1.7952363755967882\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Wire_Fitting\" ];\n\
    \"cluster_1_3\" -> \"cluster_1_end\" [ weight=\"1\", label=\"__51_3K\", width=\"0.7671440972222222\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_1_start\" -> \"cluster_1_5\" [ weight=\"1\", label=\"__72_7K\", width=\"0.7814670138888888\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_1_4\" -> \"cluster_1_3\" [ weight=\"51271\", label=\"__51_3K\", width=\"0.7671440972222222\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_1_6\" -> \"cluster_1_2\" [ weight=\"59056\", label=\"__59_1K\", width=\"0.7586805555555556\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_1_5\" -> \"cluster_1_1\" [ weight=\"69496\", label=\"__69_5K\", width=\"0.7940538194444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_1_1\" -> \"cluster_1_6\" [ weight=\"64040\", label=\"__64K\", width=\"0.66796875\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_1_2\" -> \"cluster_1_4\" [ weight=\"54655\", label=\"__54_7K\", width=\"0.7875434027777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_1_1\" -> \"cluster_1_end\" [ weight=\"1\", label=\"__5_46K\", width=\"0.798828125\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_1_2\" -> \"cluster_1_end\" [ weight=\"1\", label=\"__4_4K\", width=\"0.7076822916666666\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_1_4\" -> \"cluster_1_end\" [ weight=\"1\", label=\"__3_38K\", width=\"0.7977430555555556\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_1_5\" -> \"cluster_1_end\" [ weight=\"1\", label=\"__3_17K\", width=\"0.7554253472222222\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_1_6\" -> \"cluster_1_end\" [ weight=\"1\", label=\"__4_98K\", width=\"0.7979600694444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  }\n\
  subgraph cluster_2 {\n\
    margin=0.5;\n\
    rankdir=BT;\n\
    ranksep=0.7;\n\
    nodesep=0.3;\n\
    \"cluster_2_start\" [ shape=box, width=\"3.4991881052652993\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Start__Final_Assembly_Area\" ];\n\
  \"cluster_2_end\" [ shape=box, width=\"3.409423404269748\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"End__Final_Assembly_Area\" ];\n\
  \"cluster_2_1\" [ shape=box, width=\"2.291763517591688\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Assembly_Station\" ];\n\
  \"cluster_2_3\" [ shape=box, width=\"2.615942849053277\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Production_Complete\" ];\n\
  \"cluster_2_4\" [ shape=box, width=\"3.185307608710395\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Production_Order_Reviewed\" ];\n\
  \"cluster_2_5\" [ shape=box, width=\"2.829069561428494\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Quality_Assurance_Hold\" ];\n\
  \"cluster_2_7\" [ shape=box, width=\"2.5566978454589844\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Quality_Control_Hold\" ];\n\
  \"cluster_2_8\" [ shape=box, width=\"3.0460436079237194\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Quality_Control_Inspection\" ];\n\
  \"cluster_2_2\" [ shape=box, width=\"2.585678736368815\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Final_Testing_Station\" ];\n\
  \"cluster_2_11\" [ shape=box, width=\"2.8937000698513455\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Reject_Production_Order\" ];\n\
  \"cluster_2_9\" [ shape=box, width=\"2.8293259938557944\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Quality_Control_Release\" ];\n\
  \"cluster_2_6\" [ shape=box, width=\"3.101697709825304\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Quality_Assurance_Release\" ];\n\
  \"cluster_2_10\" [ shape=box, width=\"2.7754673428005643\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Quality_Control_Testing\" ];\n\
    \"cluster_2_start\" -> \"cluster_2_1\" [ weight=\"1\", label=\"__45_6K\", width=\"0.794921875\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_3\" -> \"cluster_2_end\" [ weight=\"1\", label=\"__46_7K\", width=\"0.7899305555555556\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_5\" -> \"cluster_2_4\" [ weight=\"11070\", label=\"__11_1K\", width=\"0.7098524305555556\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_8\" -> \"cluster_2_5\" [ weight=\"11070\", label=\"__11_1K\", width=\"0.7098524305555556\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_3\" -> \"cluster_2_4\" [ weight=\"22140\", label=\"__22_1K\", width=\"0.7593315972222222\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_7\" -> \"cluster_2_8\" [ weight=\"34447\", label=\"__34_4K\", width=\"0.8049045138888888\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_4\" -> \"cluster_2_3\" [ weight=\"46748\", label=\"__46_7K\", width=\"0.7899305555555556\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_1\" -> \"cluster_2_7\" [ weight=\"33210\", label=\"__33_2K\", width=\"0.8001302083333334\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_1\" -> \"cluster_2_end\" [ weight=\"1\", label=\"__509\", width=\"0.6586371527777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_start\" -> \"cluster_2_4\" [ weight=\"1\", label=\"__5_54K\", width=\"0.7942708333333334\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_4\" -> \"cluster_2_end\" [ weight=\"1\", label=\"__2_47K\", width=\"0.7944878472222222\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_start\" -> \"cluster_2_7\" [ weight=\"1\", label=\"__2_37K\", width=\"0.7894965277777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_7\" -> \"cluster_2_end\" [ weight=\"1\", label=\"__2_37K\", width=\"0.7894965277777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_start\" -> \"cluster_2_8\" [ weight=\"1\", label=\"__4_18K\", width=\"0.7615017361111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_8\" -> \"cluster_2_end\" [ weight=\"1\", label=\"__4_26K\", width=\"0.7973090277777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_8\" -> \"cluster_2_7\" [ weight=\"1237\", label=\"__1_24K\", width=\"0.7693142361111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_8\" -> \"cluster_2_1\" [ weight=\"1783\", label=\"__1_78K\", width=\"0.7582465277777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_8\" -> \"cluster_2_4\" [ weight=\"22140\", label=\"__22_1K\", width=\"0.7593315972222222\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_1\" -> \"cluster_2_2\" [ weight=\"39001\", label=\"__39K\", width=\"0.6671006944444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_2\" -> \"cluster_2_1\" [ weight=\"24614\", label=\"__24_6K\", width=\"0.791015625\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_start\" -> \"cluster_2_2\" [ weight=\"1\", label=\"__2_47K\", width=\"0.7944878472222222\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_2\" -> \"cluster_2_end\" [ weight=\"1\", label=\"__7_44K\", width=\"0.7810329861111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_2\" -> \"cluster_2_4\" [ weight=\"10477\", label=\"__10_5K\", width=\"0.7699652777777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_start\" -> \"cluster_2_11\" [ weight=\"1\", label=\"__1_78K\", width=\"0.7582465277777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_11\" -> \"cluster_2_end\" [ weight=\"1\", label=\"__1_78K\", width=\"0.7582465277777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_11\" -> \"cluster_2_8\" [ weight=\"1310\", label=\"__1_31K\", width=\"0.7452256944444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_5\" -> \"cluster_2_11\" [ weight=\"1310\", label=\"__1_31K\", width=\"0.7452256944444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_start\" -> \"cluster_2_9\" [ weight=\"1\", label=\"__3_02K\", width=\"0.7944878472222222\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_9\" -> \"cluster_2_end\" [ weight=\"1\", label=\"__1_96K\", width=\"0.7706163194444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_9\" -> \"cluster_2_9\" [ weight=\"1601\", label=\"__1_6K\", width=\"0.6725260416666666\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_9\" -> \"cluster_2_2\" [ weight=\"1055\", label=\"__1_06K\", width=\"0.7680121527777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_start\" -> \"cluster_2_6\" [ weight=\"1\", label=\"__1_24K\", width=\"0.7693142361111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_6\" -> \"cluster_2_8\" [ weight=\"1237\", label=\"__1_24K\", width=\"0.7693142361111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_6\" -> \"cluster_2_1\" [ weight=\"691\", label=\"__691\", width=\"0.6365017361111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_8\" -> \"cluster_2_6\" [ weight=\"691\", label=\"__691\", width=\"0.6365017361111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_start\" -> \"cluster_2_10\" [ weight=\"1\", label=\"__1_53K\", width=\"0.7690972222222222\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_10\" -> \"cluster_2_5\" [ weight=\"1310\", label=\"__1_31K\", width=\"0.7452256944444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_2_10\" -> \"cluster_2_end\" [ weight=\"1\", label=\"__218\", width=\"0.6325954861111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  }\n\
  subgraph cluster_3 {\n\
    margin=0.5;\n\
    rankdir=BT;\n\
    ranksep=0.7;\n\
    nodesep=0.3;\n\
    \"cluster_3_start\" [ shape=box, width=\"3.1347433725992837\", height=\"0.8333333333333334\", margin=\"0.06944444444444445\", label=\"Start__Metal_Stampin...\" ];\n\
  \"cluster_3_end\" [ shape=box, width=\"3.155004713270399\", height=\"0.8333333333333334\", margin=\"0.06944444444444445\", label=\"End__Metal_Stamping...\" ];\n\
  \"cluster_3_1\" [ shape=box, width=\"1.8024175431993272\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"MSM_Break\" ];\n\
  \"cluster_3_5\" [ shape=box, width=\"2.6320999993218317\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"MSM_Postprocessing\" ];\n\
  \"cluster_3_6\" [ shape=box, width=\"2.1784036424424915\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"MSM_Producing\" ];\n\
  \"cluster_3_8\" [ shape=box, width=\"2.9747450086805554\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Metal_Stamping_Machine\" ];\n\
  \"cluster_3_2\" [ shape=box, width=\"3.0150108337402344\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"MSM_Crane_not_available\" ];\n\
  \"cluster_3_3\" [ shape=box, width=\"2.168401082356771\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"MSM_Microstop\" ];\n\
  \"cluster_3_7\" [ shape=box, width=\"2.5669564141167536\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"MSM_Unknown_stop\" ];\n\
  \"cluster_3_4\" [ shape=box, width=\"3.180946562025282\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"MSM_Other_technical_issue\" ];\n\
    \"cluster_3_5\" -> \"cluster_3_end\" [ weight=\"1\", label=\"__25K\", width=\"0.6599392361111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_start\" -> \"cluster_3_8\" [ weight=\"1\", label=\"__25_6K\", width=\"0.7892795138888888\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_1\" -> \"cluster_3_5\" [ weight=\"11070\", label=\"__11_1K\", width=\"0.7098524305555556\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_8\" -> \"cluster_3_6\" [ weight=\"23247\", label=\"__23_2K\", width=\"0.7953559027777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_6\" -> \"cluster_3_1\" [ weight=\"23247\", label=\"__23_2K\", width=\"0.7953559027777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_1\" -> \"cluster_3_end\" [ weight=\"1\", label=\"__1_49K\", width=\"0.7745225694444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_start\" -> \"cluster_3_5\" [ weight=\"1\", label=\"__3_2K\", width=\"0.7029079861111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_8\" -> \"cluster_3_end\" [ weight=\"1\", label=\"__3_38K\", width=\"0.7977430555555556\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_3\" -> \"cluster_3_2\" [ weight=\"12177\", label=\"__12_2K\", width=\"0.7725694444444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_2\" -> \"cluster_3_3\" [ weight=\"12177\", label=\"__12_2K\", width=\"0.7725694444444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_3\" -> \"cluster_3_5\" [ weight=\"6642\", label=\"__6_64K\", width=\"0.7962239583333334\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_1\" -> \"cluster_3_3\" [ weight=\"12177\", label=\"__12_2K\", width=\"0.7725694444444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_start\" -> \"cluster_3_2\" [ weight=\"1\", label=\"__5_93K\", width=\"0.796875\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_2\" -> \"cluster_3_end\" [ weight=\"1\", label=\"__5_93K\", width=\"0.796875\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_7\" -> \"cluster_3_5\" [ weight=\"5535\", label=\"__5_54K\", width=\"0.7942708333333334\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_3\" -> \"cluster_3_7\" [ weight=\"5535\", label=\"__5_54K\", width=\"0.7942708333333334\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_start\" -> \"cluster_3_7\" [ weight=\"1\", label=\"__1_6K\", width=\"0.6725260416666666\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_7\" -> \"cluster_3_end\" [ weight=\"1\", label=\"__1_6K\", width=\"0.6725260416666666\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_5\" -> \"cluster_3_7\" [ weight=\"1491\", label=\"__1_49K\", width=\"0.7745225694444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_7\" -> \"cluster_3_1\" [ weight=\"1491\", label=\"__1_49K\", width=\"0.7745225694444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_4\" -> \"cluster_3_5\" [ weight=\"1710\", label=\"__1_71K\", width=\"0.7371961805555556\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_5\" -> \"cluster_3_4\" [ weight=\"1710\", label=\"__1_71K\", width=\"0.7371961805555556\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_start\" -> \"cluster_3_4\" [ weight=\"1\", label=\"__982\", width=\"0.6569010416666666\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_4\" -> \"cluster_3_8\" [ weight=\"982\", label=\"__982\", width=\"0.6569010416666666\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  }\n\
  subgraph cluster_4 {\n\
    margin=0.5;\n\
    rankdir=BT;\n\
    ranksep=0.7;\n\
    nodesep=0.3;\n\
    \"cluster_4_start\" [ shape=box, width=\"3.1357688903808594\", height=\"0.8333333333333334\", margin=\"0.06944444444444445\", label=\"Start__Plastic_Injectio...\" ];\n\
  \"cluster_4_end\" [ shape=box, width=\"3.0460041893853083\", height=\"0.8333333333333334\", margin=\"0.06944444444444445\", label=\"End__Plastic_Injectio...\" ];\n\
  \"cluster_4_1\" [ shape=box, width=\"1.6890574561225042\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"PIM_Break\" ];\n\
  \"cluster_4_5\" [ shape=box, width=\"2.5187399122450085\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"PIM_Postprocessing\" ];\n\
  \"cluster_4_7\" [ shape=box, width=\"2.0650435553656683\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"PIM_Producing\" ];\n\
  \"cluster_4_9\" [ shape=box, width=\"2.96525510152181\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Plastic_Injection_Machine\" ];\n\
  \"cluster_4_2\" [ shape=box, width=\"2.9016507466634116\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"PIM_Crane_not_available\" ];\n\
  \"cluster_4_3\" [ shape=box, width=\"2.0550409952799478\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"PIM_Microstop\" ];\n\
  \"cluster_4_8\" [ shape=box, width=\"2.4535963270399304\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"PIM_Unknown_stop\" ];\n\
  \"cluster_4_4\" [ shape=box, width=\"3.067586474948459\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"PIM_Other_technical_issue\" ];\n\
  \"cluster_4_6\" [ shape=box, width=\"2.188918855455187\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"PIM_Preparation\" ];\n\
    \"cluster_4_5\" -> \"cluster_4_end\" [ weight=\"1\", label=\"__23_2K\", width=\"0.7953559027777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_start\" -> \"cluster_4_9\" [ weight=\"1\", label=\"__24_2K\", width=\"0.7921006944444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_1\" -> \"cluster_4_5\" [ weight=\"11070\", label=\"__11_1K\", width=\"0.7098524305555556\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_9\" -> \"cluster_4_7\" [ weight=\"23247\", label=\"__23_2K\", width=\"0.7953559027777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_7\" -> \"cluster_4_1\" [ weight=\"23247\", label=\"__23_2K\", width=\"0.7953559027777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_1\" -> \"cluster_4_end\" [ weight=\"1\", label=\"__363\", width=\"0.6647135416666666\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_start\" -> \"cluster_4_7\" [ weight=\"1\", label=\"__4_66K\", width=\"0.7962239583333334\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_7\" -> \"cluster_4_end\" [ weight=\"1\", label=\"__1_64K\", width=\"0.7706163194444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_9\" -> \"cluster_4_end\" [ weight=\"1\", label=\"__2_77K\", width=\"0.7845052083333334\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_7\" -> \"cluster_4_9\" [ weight=\"1783\", label=\"__1_78K\", width=\"0.7582465277777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_3\" -> \"cluster_4_2\" [ weight=\"12177\", label=\"__12_2K\", width=\"0.7725694444444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_2\" -> \"cluster_4_3\" [ weight=\"12177\", label=\"__12_2K\", width=\"0.7725694444444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_3\" -> \"cluster_4_5\" [ weight=\"6642\", label=\"__6_64K\", width=\"0.7962239583333334\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_1\" -> \"cluster_4_3\" [ weight=\"12177\", label=\"__12_2K\", width=\"0.7725694444444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_2\" -> \"cluster_4_end\" [ weight=\"1\", label=\"__1_24K\", width=\"0.7693142361111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_start\" -> \"cluster_4_3\" [ weight=\"1\", label=\"__2_37K\", width=\"0.7894965277777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_3\" -> \"cluster_4_end\" [ weight=\"1\", label=\"__2_37K\", width=\"0.7894965277777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_2\" -> \"cluster_4_2\" [ weight=\"1237\", label=\"__1_24K\", width=\"0.7693142361111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_3\" -> \"cluster_4_8\" [ weight=\"5535\", label=\"__5_54K\", width=\"0.7942708333333334\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_8\" -> \"cluster_4_5\" [ weight=\"5535\", label=\"__5_54K\", width=\"0.7942708333333334\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_start\" -> \"cluster_4_8\" [ weight=\"1\", label=\"__3_17K\", width=\"0.7554253472222222\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_8\" -> \"cluster_4_end\" [ weight=\"1\", label=\"__3_17K\", width=\"0.7554253472222222\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_7\" -> \"cluster_4_8\" [ weight=\"1237\", label=\"__1_24K\", width=\"0.7693142361111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_start\" -> \"cluster_4_4\" [ weight=\"1\", label=\"__8_08K\", width=\"0.7931857638888888\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_4\" -> \"cluster_4_end\" [ weight=\"1\", label=\"__8_08K\", width=\"0.7931857638888888\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_6\" -> \"cluster_4_2\" [ weight=\"1237\", label=\"__1_24K\", width=\"0.7693142361111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_8\" -> \"cluster_4_6\" [ weight=\"1237\", label=\"__1_24K\", width=\"0.7693142361111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_start\" -> \"cluster_4_6\" [ weight=\"1\", label=\"__363\", width=\"0.6647135416666666\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_6\" -> \"cluster_4_1\" [ weight=\"363\", label=\"__363\", width=\"0.6647135416666666\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  }\n\
  subgraph cluster_5 {\n\
    margin=0.5;\n\
    rankdir=BT;\n\
    ranksep=0.7;\n\
    nodesep=0.3;\n\
    \"cluster_5_start\" [ shape=box, width=\"3.152952618069119\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Start__Production_Start\" ];\n\
  \"cluster_5_end\" [ shape=box, width=\"3.063187917073568\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"End__Production_Start\" ];\n\
  \"cluster_5_1\" [ shape=box, width=\"3.1422201792399087\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Production_Order_Received\" ];\n\
  \"cluster_5_2\" [ shape=box, width=\"2.2209775712754993\", height=\"0.6111111111111112\", margin=\"0.06944444444444445\", label=\"Production_Start\" ];\n\
    \"cluster_5_start\" -> \"cluster_5_1\" [ weight=\"1\", label=\"__83_2K\", width=\"0.7970920138888888\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_5_2\" -> \"cluster_5_end\" [ weight=\"1\", label=\"__78_3K\", width=\"0.7855902777777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_5_1\" -> \"cluster_5_2\" [ weight=\"78263\", label=\"__78_3K\", width=\"0.7855902777777778\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_5_1\" -> \"cluster_5_end\" [ weight=\"1\", label=\"__4_95K\", width=\"0.7966579861111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  }\n\
    \"cluster_0_8\" -> \"cluster_1_5\" [ weight=\"11070\", label=\"2_links\", width=\"1.2955729166666667\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_0_12\" -> \"cluster_1_1\" [ weight=\"1710\", label=\"2_links\", width=\"1.3611111111111112\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_5\" -> \"cluster_1_5\" [ weight=\"11070\", label=\"__11_1K___11_1K\", width=\"1.2955729166666667\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_3_4\" -> \"cluster_1_2\" [ weight=\"982\", label=\"__982___982\", width=\"1.1388888888888888\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_5\" -> \"cluster_1_5\" [ weight=\"11070\", label=\"__11_1K___11_1K\", width=\"1.2955729166666667\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_4\" -> \"cluster_1_5\" [ weight=\"1965\", label=\"__1_97K___1_97K\", width=\"1.3897569444444444\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_4_1\" -> \"cluster_1_6\" [ weight=\"363\", label=\"__363___363\", width=\"1.1545138888888888\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_1_3\" -> \"cluster_2_1\" [ weight=\"50580\", label=\"__50_6K___50_6K\", width=\"1.4518229166666667\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_5_2\" -> \"cluster_0_13\" [ weight=\"42808\", label=\"__42_8K___42_8K\", width=\"1.4661458333333333\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_5_2\" -> \"cluster_0_12\" [ weight=\"691\", label=\"__691___691\", width=\"1.0980902777777777\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_5_2\" -> \"cluster_0_10\" [ weight=\"2474\", label=\"__2_47K___2_47K\", width=\"1.4574652777777777\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_5_1\" -> \"cluster_0_5\" [ weight=\"1601\", label=\"__1_6K___1_6K\", width=\"1.2213541666666667\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_5_1\" -> \"cluster_0_12\" [ weight=\"2328\", label=\"__2_33K___2_33K\", width=\"1.4791666666666667\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_5_2\" -> \"cluster_0_11\" [ weight=\"691\", label=\"__691___691\", width=\"1.0980902777777777\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_5_2\" -> \"cluster_3_8\" [ weight=\"11070\", label=\"__11_1K___11_1K\", width=\"1.2955729166666667\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_5_2\" -> \"cluster_4_9\" [ weight=\"11070\", label=\"__11_1K___11_1K\", width=\"1.2955729166666667\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_5_2\" -> \"cluster_4_3\" [ weight=\"2365\", label=\"__2_37K___2_37K\", width=\"1.4592013888888888\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_5_1\" -> \"cluster_4_8\" [ weight=\"2474\", label=\"__2_47K___2_47K\", width=\"1.4574652777777777\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_5_1\" -> \"cluster_4_4\" [ weight=\"1310\", label=\"__1_31K___1_31K\", width=\"1.3671875\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  \"cluster_5_2\" -> \"cluster_4_7\" [ weight=\"1783\", label=\"__1_78K___1_78K\", width=\"1.4032118055555556\", height=\"0.2777777777777778\", fontname=\"Helvetica\", fontsize=\"15pt\"];\n\
  }\n\
  ";

  return 0;
}

// XFAIL: True
// RUN: clink --build-only --database={%t} --debug --parse-c=cscope {%s}
