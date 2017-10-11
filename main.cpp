#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}

/*int MainWindow::sdp_search_spp(sdp_session_t *sdp, uint8_t *channel)
{
    int result = -1;
    sdp_list_t *srch, *attrs, *rsp;
    uuid_t svclass;
    uint16_t attr;
    int err;

    if (!sdp)
        return -1;

    sdp_uuid16_create(&svclass, SERIAL_PORT_SVCLASS_ID);
    srch = sdp_list_append(NULL, &svclass);

    attr = SDP_ATTR_PROTO_DESC_LIST;
    attrs = sdp_list_append(NULL, &attr);

    err = sdp_service_search_attr_req(sdp, srch, SDP_ATTR_REQ_INDIVIDUAL, attrs, &rsp);
    if (err)
        return -1;

    sdp_list_t *r = rsp;

   // go through each of the service records
   for (; r; r = r->next ) {
       sdp_record_t *rec = (sdp_record_t*) r->data;
       sdp_list_t *proto_list;

       // get a list of the protocol sequences
       if( sdp_get_access_protos( rec, &proto_list ) == 0 ) {
       sdp_list_t *p = proto_list;

       // go through each protocol sequence
       for( ; p ; p = p->next ) {
           sdp_list_t *pds = (sdp_list_t*)p->data;

           // go through each protocol list of the protocol sequence
           for( ; pds ; pds = pds->next ) {

               // check the protocol attributes
               sdp_data_t *d = (sdp_data_t*)pds->data;
               int proto = 0;
               for( ; d; d = d->next ) {
                   switch( d->dtd ) {
                       case SDP_UUID16:
                       case SDP_UUID32:
                       case SDP_UUID128:
                           proto = sdp_uuid_to_proto( &d->val.uuid );
                           break;
                       case SDP_UINT8:
                           if( proto == RFCOMM_UUID ) {
                               *channel=d->val.int8;
                               result = 0;
                           }
                           break;
                   }
               }
           }
           sdp_list_free( (sdp_list_t*)p->data, 0 );
       }
       sdp_list_free( proto_list, 0 );
       }
       sdp_record_free( rec );
   }
   sdp_close(sdp);
   return result;
}

bool MainWindow::bindRFComm(const char* dest)
{
     bdaddr_t my_bdaddr_any = {0, 0, 0, 0, 0, 0};
     bdaddr_t target;
     sdp_session_t *session = 0;
     str2ba(dest, &target );

     //connect to the SDP server running on the remote machine
     session = sdp_connect( &my_bdaddr_any, &target, 0 );
     u_int8_t channel;

     if(sdp_search_spp(session,&channel) == 0)
     {
          printf("Found rfcomm on channel: %d\n",channel);
          sprintf(log_buffer,"rfcomm bind 0 %s %d",dest,channel);
          printf("%s\n",log_buffer);
          std::string bind = exec(log_buffer);
          printf("%s\n",bind.c_str());
     }
     else
     {
          printf("Rfcomm service not found\n");
          return false;
     }

    return true;
}*/
