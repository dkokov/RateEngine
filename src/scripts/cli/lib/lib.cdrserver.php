<?php

//require('lib.mycc.php');

function insert_cdr($arrXml)
{
    $conn = pg_connect("host=127.0.0.1 port=5432 dbname=fs_cdrs user=global password=_cfg.access");

    if(empty($arrXml['variables']['sip_h_X-freetdm-rdnis-screen'])) $arrXml['variables']['sip_h_X-freetdm-rdnis-screen'] = 0;
    if(empty($arrXml['variables']['sip_h_X-freetdm-rdnis-presentation'])) $arrXml['variables']['sip_h_X-freetdm-rdnis-presentation'] = 0;
    if(empty($arrXml['variables']['sip_h_X-freetdm-rdnis-nadi'])) $arrXml['variables']['sip_h_X-freetdm-rdnis-nadi'] = 0;
    if(empty($arrXml['variables']['sip_h_X-freetdm-clg-nadi'])) $arrXml['variables']['sip_h_X-freetdm-clg-nadi'] = 0;
    if(empty($arrXml['variables']['sip_h_X-freetdm-cld-nadi'])) $arrXml['variables']['sip_h_X-freetdm-cld-nadi'] = 0;
    if(empty($arrXml['variables']['sip_h_X-freetdm-presentation'])) $arrXml['variables']['sip_h_X-freetdm-presentation'] = 0;
    if(empty($arrXml['variables']['sip_h_X-freetdm-screen'])) $arrXml['variables']['sip_h_X-freetdm-screen'] = 0;
    if(empty($arrXml['variables']['sip_h_X-freetdm-rdnis'])) $arrXml['variables']['sip_h_X-freetdm-rdnis'] = '';
    if(empty($arrXml['variables']['sip_local_network_addr'])) $arrXml['variables']['sip_local_network_addr'] = '';
    if(empty($arrXml['variables']['sip_contact_uri'])) $arrXml['variables']['sip_contact_uri'] = '';
    if(empty($arrXml['variables']['sip_to_uri'])) $arrXml['variables']['sip_to_uri'] = '';
    if(empty($arrXml['variables']['sip_full_via'])) $arrXml['variables']['sip_full_via'] = '';
    if(empty($arrXml['variables']['remote_media_ip'])) $arrXml['variables']['remote_media_ip'] = '';
    if(empty($arrXml['variables']['remote_media_port'])) $arrXml['variables']['remote_media_port'] = 0;
    if(empty($arrXml['variables']['maxsec'])) $arrXml['variables']['maxsec'] = -1;
    if(empty($arrXml['variables']['limit_usage'])) $arrXml['variables']['limit_usage'] = -1;

    if(empty($arrXml['call-stats']['audio']['inbound']['packet_count'])) $arrXml['call-stats']['audio']['inbound']['packet_count'] = -1;
    if(empty($arrXml['call-stats']['audio']['inbound']['media_packet_count'])) $arrXml['call-stats']['audio']['inbound']['media_packet_count'] = -1;
    if(empty($arrXml['call-stats']['audio']['inbound']['skip_packet_count'])) $arrXml['call-stats']['audio']['inbound']['skip_packet_count'] = -1;
    if(empty($arrXml['call-stats']['audio']['inbound']['jitter_packet_count'])) $arrXml['call-stats']['audio']['inbound']['jitter_packet_count'] = -1;
    if(empty($arrXml['call-stats']['audio']['inbound']['dtmf_packet_count'])) $arrXml['call-stats']['audio']['inbound']['dtmf_packet_count'] = -1;
    if(empty($arrXml['call-stats']['audio']['inbound']['cng_packet_count'])) $arrXml['call-stats']['audio']['inbound']['cng_packet_count'] = -1;
    if(empty($arrXml['call-stats']['audio']['inbound']['flush_packet_count'])) $arrXml['call-stats']['audio']['inbound']['flush_packet_count'] = -1;
    if(empty($arrXml['call-stats']['audio']['inbound']['largest_jb_size'])) $arrXml['call-stats']['audio']['inbound']['largest_jb_size'] = -1;
    if(empty($arrXml['call-stats']['audio']['inbound']['jitter_min_variance'])) $arrXml['call-stats']['audio']['inbound']['jitter_min_variance'] = -1;
    if(empty($arrXml['call-stats']['audio']['inbound']['jitter_max_variance'])) $arrXml['call-stats']['audio']['inbound']['jitter_max_variance'] = -1;
    if(empty($arrXml['call-stats']['audio']['inbound']['jitter_loss_rate'])) $arrXml['call-stats']['audio']['inbound']['jitter_loss_rate'] = -1;
    if(empty($arrXml['call-stats']['audio']['inbound']['jitter_burst_rate'])) $arrXml['call-stats']['audio']['inbound']['jitter_burst_rate'] = -1;
    if(empty($arrXml['call-stats']['audio']['inbound']['mean_interval'])) $arrXml['call-stats']['audio']['inbound']['mean_interval'] = -1;
    if(empty($arrXml['call-stats']['audio']['inbound']['flaw_total'])) $arrXml['call-stats']['audio']['inbound']['flaw_total'] = -1;
    if(empty($arrXml['call-stats']['audio']['inbound']['mos'])) $arrXml['call-stats']['audio']['inbound']['mos'] = -1;
    if(empty($arrXml['call-stats']['audio']['inbound']['quality_percentage'])) $arrXml['call-stats']['audio']['inbound']['quality_percentage'] = -1;
    if(empty($arrXml['call-stats']['audio']['error-log']['error-period']['duration-msec'])) $arrXml['call-stats']['audio']['error-log']['error-period']['duration-msec'] = -1;


    $query = "insert into cdrs 
              (call_uid,
              src_context,
              incoming_channel,
              billsec,
              duration,
              src,
              dst,
              ts,
              clg_nadi,
              cld_nadi,
              outgoing_channel,
              screen,
              presentation,
              hangup_cause,
              originate_disposition,
              hangup_cause_q850,
              dst_tgroup,
              src_tgroup,
              incoming_codec,
              outgoing_codec,
              uduration,
              billusec,
              rdnis,
              rdnis_nadi,
              rdnis_screen,rdnis_presentation,
              local_network_addr,contact_uri,to_uri,full_via,
              remote_media_ip,remote_media_port,maxsec,call_usage,account_code,
              packet_count,media_packet_count,skip_packet_count,jitter_packet_count,dtmf_packet_count,cng_packet_count,
              flush_packet_count,largest_jb_size,jitter_min,jitter_max,jitter_loss_rate,jitter_burst_rate,
              mean_interval,flaw_total,mos,quality_percentage,error_period_msec,gps) 
              values('".$arrXml['variables']['uuid']."','".
              $arrXml['callflow']['caller_profile']['context']."','".
              $arrXml['callflow']['caller_profile']['chan_name']."',".
              $arrXml['variables']['billsec'].",".
              $arrXml['variables']['duration'].",'".
              $arrXml['variables']['ANI']."','".
              $arrXml['variables']['DNIS']."','".
              date("Y-m-d H:i:s",
              $arrXml['variables']['start_epoch'])."',".
              $arrXml['variables']['sip_h_X-freetdm-clg-nadi'].",".
              $arrXml['variables']['sip_h_X-freetdm-cld-nadi'].",'".
              $arrXml['callflow']['caller_profile']['chan_name']."',".
              $arrXml['variables']['sip_h_X-freetdm-screen'].",".
              $arrXml['variables']['sip_h_X-freetdm-presentation'].",'".
              $arrXml['variables']['hangup_cause']."','".
              $arrXml['variables']['originate_disposition']."',".
              $arrXml['variables']['hangup_cause_q850'].",'".
              $arrXml['variables']['dst_tgroup']."','".
              $arrXml['variables']['src_tgroup']."','".
              $arrXml['variables']['read_codec']."','".
              $arrXml['variables']['write_codec']."',".
              $arrXml['variables']['uduration'].",".
              $arrXml['variables']['billusec'].",'".
              $arrXml['variables']['sip_h_X-freetdm-rdnis']."',".
              $arrXml['variables']['sip_h_X-freetdm-rdnis-nadi'].",".
              $arrXml['variables']['sip_h_X-freetdm-rdnis-screen'].",".
              $arrXml['variables']['sip_h_X-freetdm-rdnis-presentation'].",'".
              $arrXml['variables']['sip_local_network_addr']."','".
              $arrXml['variables']['sip_contact_uri']."','".
              $arrXml['variables']['sip_to_uri']."','".
              $arrXml['variables']['sip_full_via']."','".
              $arrXml['variables']['remote_media_ip']."',".
              $arrXml['variables']['remote_media_port'].",".
              $arrXml['variables']['maxsec'].",".
              $arrXml['variables']['limit_usage'].",'".
              $arrXml['variables']['account_code']."',".
              $arrXml['call-stats']['audio']['inbound']['packet_count'].",".
              $arrXml['call-stats']['audio']['inbound']['media_packet_count'].",".
              $arrXml['call-stats']['audio']['inbound']['skip_packet_count'].",".
              $arrXml['call-stats']['audio']['inbound']['jitter_packet_count'].",".
              $arrXml['call-stats']['audio']['inbound']['dtmf_packet_count'].",".
              $arrXml['call-stats']['audio']['inbound']['cng_packet_count'].",".
              $arrXml['call-stats']['audio']['inbound']['flush_packet_count'].",".
              $arrXml['call-stats']['audio']['inbound']['largest_jb_size'].",".
              $arrXml['call-stats']['audio']['inbound']['jitter_min_variance'].",".
              $arrXml['call-stats']['audio']['inbound']['jitter_max_variance'].",".
              $arrXml['call-stats']['audio']['inbound']['jitter_loss_rate'].",".
              $arrXml['call-stats']['audio']['inbound']['jitter_burst_rate'].",".
              $arrXml['call-stats']['audio']['inbound']['mean_interval'].",".
              $arrXml['call-stats']['audio']['inbound']['flaw_total'].",".
              $arrXml['call-stats']['audio']['inbound']['mos'].",".
              $arrXml['call-stats']['audio']['inbound']['quality_percentage'].",".
              $arrXml['call-stats']['audio']['error-log']['error-period']['duration-msec'].",'".$arrXml['variables']['sip_h_X-UserCoordinate']."')";
    $res = pg_query($conn,$query);
//    file_put_contents("/tmp/debug", $query . "\n\n",FILE_APPEND);
    pg_close($conn);

//    if($arrXml['variables']['maxsec'] > 0) myCC_term($arrXml['variables']['uuid'],$arrXml['variables']['billsec'],$arrXml['variables']['duration']);
}

/*
function objectsIntoArray($arrObjData, $arrSkipIndices = array())
{
    $arrData = array();
   
    // if input is object, convert into array
    if (is_object($arrObjData)) {
        $arrObjData = get_object_vars($arrObjData);
    }
   
    if (is_array($arrObjData)) {
        foreach ($arrObjData as $index => $value) {
            if (is_object($value) || is_array($value)) {
                $value = objectsIntoArray($value, $arrSkipIndices); // recursive call
            }
            if (in_array($index, $arrSkipIndices)) {
                continue;
            }
            $arrData[$index] = $value;
        }
    }
    return $arrData;
}

    $xmlStr = str_replace(":","_",$_POST['cdr']);
    $xmlObj = simplexml_load_string($xmlStr);
    $arrXml = objectsIntoArray($xmlObj);
*/

 $arrXml['variables']['uuid'] = mt_rand(1000,9999).'-test-'.mt_rand(100000,999999).'-dddd-'.mt_rand(1000,9999);
 
 $dur = mt_rand(5000000,3600000000);
 $bill = $dur - 5000000;
 
 $arrXml['variables']['billsec']  = round($bill/1000000);
 $arrXml['variables']['duration'] = round($dur/1000000);
 
 $arrXml['variables']['ANI']  = '35910000000';
 $arrXml['variables']['DNIS'] = '35910123456';
 $arrXml['variables']['start_epoch']  = mktime(); 
 $arrXml['variables']['hangup_cause'] = 'NORMAL CLEARING';
 $arrXml['variables']['hangup_cause_q850'] = 16;
 $arrXml['variables']['uduration'] = $dur;
 $arrXml['variables']['billusec']  = $bill;
 
 $arrXml['variables']['dst_tgroup']   = NULL;
 $arrXml['variables']['src_tgroup']   = NULL;
 $arrXml['variables']['read_codec']   = NULL;
 $arrXml['variables']['write_codec']  = NULL;
 $arrXml['variables']['account_code'] = NULL;
 $arrXml['variables']['originate_disposition']  = NULL;
 $arrXml['variables']['sip_h_X-UserCoordinate'] = NULL;

 insert_cdr($arrXml);
?>
