/*
 ============================================================================
 Name        : ALTOclient.c
 Author      : T. Ewald <ewald@nw.neclab.eu>
 Version     : 243
 Proprietary : NEC Europe Ltd. PROPRIETARY INFORMATION
			   This software is supplied under the terms of a license
			   agreement or nondisclosure agreement with NEC Europe Ltd. and
			   may not be copied or disclosed except in accordance with the
			   terms of that agreement. The software and its source code
			   contain valuable trade secrets and confidential information
			   which have to be maintained in confidence.
			   Any unauthorized publication, transfer to third parties or
			   duplication of the object or source code - either totally or in
			   part - is prohibited.
 Copyright	 : Copyright (c) 2004 NEC Europe Ltd. All Rights Reserved.
			   NEC Europe Ltd. DISCLAIMS ALL WARRANTIES, EITHER EXPRESS OR
			   IMPLIED, INCLUDING BUT NOT LIMITED TO IMPLIED WARRANTIES OF
			   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND THE
			   WARRANTY AGAINST LATENT DEFECTS, WITH RESPECT TO THE PROGRAM
			   AND THE ACCOMPANYING DOCUMENTATION.
			   No Liability For Consequential Damages IN NO EVENT SHALL NEC
			   Europe Ltd., NEC Corporation OR ANY OF ITS SUBSIDIARIES BE
			   LIABLE FOR ANY DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,
			   DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS
			   OF INFORMATION, OR OTHER PECUNIARY LOSS AND INDIRECT,
			   CONSEQUENTIAL, INCIDENTAL, ECONOMIC OR PUNITIVE DAMAGES) ARISING
			   OUT OF THE USE OF OR INABILITY TO USE THIS PROGRAM, EVEN IF NEC
			   Europe Ltd. HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 Modification: THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 Description : First try of the ALTO client
 ============================================================================
 */

#include "ALTOclient.h"
#include "ALTOclient_impl.h"


/*
 * 		Here the reference to the accessible DBs is set
 */
altoDbPtr ALTO_DB_req;		// Pointer to the ALTO DB for the Request
altoDbPtr ALTO_DB_res;		// Pointer to the ALTO DB for the Resposne

xmlDocPtr ALTO_XML_req;		// Pointer to the XML for the Request
xmlDocPtr ALTO_XML_res;		// Pointer to the XML for the Request


// This is the varaiable where the ALTO server can be found
char alto_server_url[256];

// And this is the struct where the XML buffer for CURL is cached
//struct curl_reply_buffer_t alto_rep_buf = {ALTO_REP_BUF_SIZE,0,""};

static char alto_reply_buf_nano[ALTO_REP_BUF_SIZE];

/*
 * 	Function to set the actual ALTO server for configuration
 */
int set_ALTO_server(char * string){
	// Sanity check
	if(string == NULL){
		fprintf(stdout, "Nothing to set here\n");
		return -1;
	}
	strncpy(alto_server_url, string, strlen(string));
	return 1;
}

/*
 * 	get the address from the actual set ALTO server;
 */
char *get_ALTO_server(void){
	fprintf(stdout, "The ALTO server is set to: %s \n", alto_server_url);
	return (char *) alto_server_url;
}








/*
 * 	Func:					Convert the "Baschtl" notation into a readable
 * 							format (get IP address)
 * 	in:		*host_string	Pointer to the string to convert
 * 	return:	IP				Struct where the IP and prefix is encoded
 */
struct in_addr get_ALTO_host_IP(char * host_string){
	struct in_addr IP;
	#ifdef WIN32
	char * str_buff = new char[strlen(host_string)];
	#else
	char str_buff[strlen(host_string)];
	#endif
	strncpy(str_buff,host_string,strlen(host_string));
	char *result = NULL;
	result = strchr(str_buff, '/');
	if(result != NULL) {
		*result = 0;
		IP.s_addr = inet_addr(str_buff);
		return IP;
	}if(inet_addr(host_string) != (-1)){
		IP.s_addr = inet_addr(host_string);
		return IP;
	}
	printf("No IP found\n!!!");
	#ifdef WIN32
	delete [] str_buff;
	#endif
	return IP;
}




/*
 * 	Func:			Convert the "Baschtl" notation into a readable format
 * 					(get prefix)
 * 	in:		*host	Pointer to the string to convert
 * 	return:	port	struct where the IP and prefix is encoded
 */
int16_t get_ALTO_host_mask(char * host_string){
	#ifdef WIN32
	char * str_buff = new char[strlen(host_string)];
	#else
	char str_buff[strlen(host_string)];
	#endif
	int16_t res;
	strncpy(str_buff,host_string,strlen(host_string));
	char *result = NULL;
	result = strchr(str_buff, '/');
	if(result != NULL) {
		result++;
		res = atoi(result);
		#ifdef WIN32
		delete [] str_buff;
		#endif
		return res;
	}
	// if it can't be found, it was a single host so mask is 32 bit
	#ifdef WIN32
	delete [] str_buff;
	#endif
	return 32;
}



/*
 *  Func:	Create an ALTO XML request from a given DB
 *
 *  in:		*db			the pointer to the DB with the elements
			rc_host		the in_addr of the requesting host
 *  ret:	XML_doc		the XML where the request is stored in
 */
xmlDocPtr alto_create_request_XML(struct alto_db_t * db, struct in_addr rc_host, int pri_rat, int sec_rat){

	// Creates a new document
	// <?xml version="1.0" encoding="UTF-8"?>
	xmlDocPtr doc = NULL;       /* document pointer */
	doc = xmlNewDoc(BAD_CAST "1.0");

	// Create the root node and name it with the correct name space
	// <alto xmlns='urn:ietf:params:xml:ns:p2p:alto'>
	// Dirty!!! Because NS is here an attribute
	// [TODO namespace declaration]
	xmlNodePtr root_node = NULL;
	root_node = xmlNewNode(NULL, BAD_CAST "alto");
    xmlNewNs(root_node, BAD_CAST "urn:ietf:params:xml:ns:p2p:alto",NULL);

    // link it to the document
    xmlDocSetRootElement(doc, root_node);

    // Creates a DTD declaration. Isn't mandatory.
    // [TODO introduce DTDs by time]
//	xmlDtdPtr dtd = NULL;       /* DTD pointer */
//  dtd = xmlCreateIntSubset(doc, BAD_CAST "root", NULL, BAD_CAST "tree2.dtd");

	// Here create the group rating request (node_GRR)
    // <group_rating_request db_version='1234'>\n"
    xmlNodePtr node_GRR = NULL;
    node_GRR = xmlNewChild(root_node, NULL, BAD_CAST "group_rating_request", NULL);
    xmlNewProp(node_GRR, BAD_CAST "db_version", BAD_CAST "1234");

    // Now create the primary rating criteria
    // <pri_ratcrit crit='pref'/>
    xmlNodePtr node_PRI = NULL;
    node_PRI = xmlNewChild(node_GRR, NULL, BAD_CAST "pri_ratcrit", NULL);
    if(pri_rat == REL_PREF){
    	xmlNewProp(node_PRI, BAD_CAST "crit", BAD_CAST "pref");
    }else if(pri_rat == TOP_DIST){
    	xmlNewProp(node_PRI, BAD_CAST "crit", BAD_CAST "dist");
    }else if(pri_rat == MIN_BOUN){
    	xmlNewProp(node_PRI, BAD_CAST "crit", BAD_CAST "lat");
    }


    // Add the additional rating criteria
	if((sec_rat & REL_PREF) == REL_PREF){
		xmlNodePtr node_SEC1 = NULL;
		node_SEC1 = xmlNewChild(node_GRR, NULL, BAD_CAST "fur_ratcrit", NULL);
		xmlNewProp(node_SEC1, BAD_CAST "crit", BAD_CAST "pref");
	}
	if((sec_rat & TOP_DIST) == TOP_DIST){
		xmlNodePtr node_SEC2 = NULL;
		node_SEC2 = xmlNewChild(node_GRR, NULL, BAD_CAST "fur_ratcrit", NULL);
		xmlNewProp(node_SEC2, BAD_CAST "crit", BAD_CAST "dist");
	}
	if((sec_rat & MIN_BOUN) == MIN_BOUN){
		xmlNodePtr node_SEC4 = NULL;
		node_SEC4 = xmlNewChild(node_GRR, NULL, BAD_CAST "fur_ratcrit", NULL);
		xmlNewProp(node_SEC4, BAD_CAST "crit", BAD_CAST "lat");
	}


    // Now create the source of the request
    // <rc_hla><ipprefix version='4' prefix='195.37.70.39/32'/></rc_hla>
    xmlNodePtr node_RC_HLA = NULL;
    node_RC_HLA = xmlNewChild(node_GRR, NULL, BAD_CAST "rc_hla", NULL);

    // and within the actual source
    // <ipprefix version='4' prefix='195.37.70.39/32'/>
    xmlNodePtr node_IPP = NULL;
    node_IPP = xmlNewChild(node_RC_HLA, NULL, BAD_CAST "ipprefix", NULL);
    xmlNewProp(node_IPP, BAD_CAST "version", BAD_CAST "4");
//  xmlNewProp(node_IPP, BAD_CAST "prefix", BAD_CAST "195.37.70.39/32");
    char rc_host_str[256];
    strcpy(rc_host_str, inet_ntoa(rc_host));
    // TODO: really dirty, but it should work
    strcat(rc_host_str,"/32");
    xmlNewProp(node_IPP, BAD_CAST "prefix", BAD_CAST rc_host_str);

    // Now prepare the request
    // <cnd_hla>
    xmlNodePtr node_HLA = NULL;
    node_HLA = xmlNewChild(node_GRR, NULL, BAD_CAST "cnd_hla", NULL);

    // So far it went quite well, now try to create the host list
    // from the given ALTO_LIST
    // so create something like this:
    // <ipprefix version='4' prefix='195.37.70.39/32'/>
    char tmp_buff[256];
    struct alto_db_element_t * cur;
    cur = db->first;
    while(cur->next != NULL){
    	node_IPP = xmlNewChild(node_HLA, NULL, BAD_CAST "ipprefix", NULL);
    	xmlNewProp(node_IPP, BAD_CAST "version", BAD_CAST "4");
    	sprintf(tmp_buff,"%s/%d",inet_ntoa(cur->host), cur->host_mask);
    	xmlNewProp(node_IPP, BAD_CAST "prefix", BAD_CAST tmp_buff);
    	cur = cur->next;
    }

    // Dump for checking
	#ifdef WIN32
    // void
	#else
    xmlDocDump(stdout, doc);
    #endif


    // Free the global variables that may have been allocated by the parser.
    xmlCleanupParser();

    // return the finsihed XML
    return doc;
}

// libcurl is now obsolete!
#if 0

// this function will be registered with curl as a handler, which
// copies the http reply to a the buffer
size_t curl_copy_reply_to_buf(void *ptr,size_t size,size_t nmemb,void *stream){
    size_t realsize = size * nmemb;
    struct curl_reply_buffer_t *crb = (struct curl_reply_buffer_t *)stream;
    // error: new chunk plus trailing zero would not fit into remaining buffer
    if( (realsize+1) > (crb->size - crb->fill) ) return 0;
    memcpy( &crb->buffer[crb->fill], ptr, realsize );
    crb->fill += realsize;
    crb->buffer[crb->fill] = 0;
    return realsize;
}
uint16_t prefixes = 0;



xmlDocPtr query_ALTO_server_curl(xmlDocPtr doc, char* ALTO_server_URL){
	xmlDocPtr ALTO_XML_response;

	// starting here the ALTO list will be send out.....
	CURL *curl;
	CURLcode res;
	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;

	curl = curl_easy_init();
	if(!curl) {
		printf("Error: couldn't get a handle from curl_easy_init(). abort.\n");
		return NULL;
	}

//	printf("Will send HTTP POST to %s\nwith form data:\n\n%s\n", alto_server_url, ALTO_XML_query);

	// prepare the buffer to be send
	xmlChar*  doctxt;
	int       doclen;
	xmlDocDumpFormatMemoryEnc(doc,&doctxt,&doclen,"utf-8",1);

	// URL that receives this POST
	curl_easy_setopt(curl, CURLOPT_URL, ALTO_server_URL);

	// add form data
	curl_formadd(&formpost,
				 &lastptr,
				 CURLFORM_COPYNAME, "alto_xml_request",
//				 CURLFORM_COPYCONTENTS, ALTO_XML_query,
				 CURLFORM_COPYCONTENTS, doctxt,
				 CURLFORM_END);

	curl_formadd(&formpost,
			     &lastptr,
			     CURLFORM_COPYNAME, "action",
			     CURLFORM_COPYCONTENTS, "submit",
			     CURLFORM_END);

	curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

	// we do not want the reply written to stdout but to our buffer
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_copy_reply_to_buf);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &alto_rep_buf);

	// do it!
	res = curl_easy_perform(curl);

	// always cleanup
	curl_easy_cleanup(curl);

	// then cleanup the form post chain
	curl_formfree(formpost);

//  printf("result of curl_easy_perform() is: %i\n", res);
//  printf("received %i octetts. the buffer is:\n\n%s\nthat's all. bye.\n",alto_rep_buf.fill,alto_rep_buf.buffer);


	// and last but nor least, transform it into an XML doc
	ALTO_XML_response = xmlRecoverMemory(alto_rep_buf.buffer,sizeof(alto_rep_buf.buffer));		// <- for getting XML from memory

	return ALTO_XML_response;
}


#endif

#define POST_BOUNDARY "---------------------------12408751047121013601852724877"

void POST_add(char* buf, const char* name, const char* value) {
	sprintf(buf+strlen(buf), "--"POST_BOUNDARY"\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n%s\r\n", name, value);
}

void POST_end(char* buf) {
	sprintf(buf+strlen(buf), "--"POST_BOUNDARY"--\r\n\r\n");
}

void* POST_send(const char* url, const char* data) {
	void* ctx;
	char contentType[512] = "multipart/form-data; boundary="POST_BOUNDARY;
	char* ct = contentType;

	ctx = xmlNanoHTTPMethod(url, "POST", data, &ct, NULL, strlen(data));

	free(ct);
	return ctx;
}

// nano
xmlDocPtr ALTO_request_to_server(xmlDocPtr doc, char* endPoint){
	xmlDocPtr result = NULL;
	int       size;
	xmlChar*  doctxt;
	int       doclen;
	int       bytesRead;
	size = 0;
	char*	data;
	size_t  dataLen = 0;
	void*	ctx = NULL;
//	int		errorcode = 0;
//	FILE*	f = NULL;

	xmlNanoHTTPInit();
	xmlDocDumpFormatMemoryEnc(doc,&doctxt,&doclen,"utf-8",1);

	dataLen = doclen + 2048;
	data = malloc(dataLen);
	memset(data, 0, dataLen);

	// build the mime multipart contents
	POST_add(data, "alto_xml_request", doctxt);
	POST_add(data, "action", "submit");
	POST_end(data);

	//printf("data:\n\n%s", data);

	printf("POST it\n");

	// send it
	ctx = POST_send(endPoint, data);

	free(data);
	data = NULL;

	printf("after POST it\n");
/*
	char output[10024];
	memset(output, 0, 10024);

	bytesRead = xmlNanoHTTPRead(ctx,&output,10024);
//	output[bytesRead] = '\n';
*/
	memset(alto_reply_buf_nano, 0, ALTO_REP_BUF_SIZE);

	bytesRead = xmlNanoHTTPRead(ctx, &alto_reply_buf_nano, ALTO_REP_BUF_SIZE);
	printf("xmlNanoHTTPRead: %d bytes read.\n", bytesRead);
	printf("ALTO reply (%d bytes):\n\n%s", bytesRead, alto_reply_buf_nano);

	// dump to file
/*	f = fopen("http_reply.txt", "wt");
	fwrite(output, 1, strlen(output), f);
	fclose(f);*/

	result = xmlRecoverMemory(alto_reply_buf_nano, bytesRead);
//
// TODO: PushParser doesn't work yet somehow..
//
/*
	xmlParserCtxtPtr pushCtx = NULL;
	pushCtx = xmlCreatePushParserCtxt(NULL, NULL, output, bytesRead, NULL);
//	xmlInitParserCtxt(pushCtx);

	if (!pushCtx) {
		printf("ERROR: xmlCreatePushParserCtxt failed!\n");
	} else {
		printf("xmlCreatePushParserCtxt ok.\n");
	}

	while (bytesRead) {
		if(xmlParseChunk(pushCtx, output, bytesRead, 0) != 0) {
			printf("ERROR: xmlParseChunk failed!\n");
		} else {
			printf("xmlParseChunk ok.\n");
		}
		bytesRead = xmlNanoHTTPRead(ctx,&output,10024);
		printf("xmlNanoHTTPRead: %d bytes read.\n", bytesRead);
	}

	printf("Finalizing...\n");
	errorcode = xmlParseChunk(pushCtx, output, 0, 1);
	if(errorcode) {
		printf("ERROR: Final xmlParseChunk failed! errorcode=%d\n", errorcode);
	} else {
		printf("Final xmlParseChunk ok.\n");
	}

	result = pushCtx->myDoc;
	xmlFreeParserCtxt(pushCtx);
*/

	if (!result) {
		printf("ERROR: XML parsing failed (result == NULL)!\n");
	} else {
		printf("XML parsing ok.\n");
	}

	xmlNanoHTTPClose(ctx);
	return result;
}




/*
 *
 * 		HERE is the magic for the internal DB management
 *
 */

/*
 * 	Initialize a ALTO DB structure
 */
struct alto_db_t * alto_db_init(void){
	fprintf(stdout, "Initialize an ALTO database! \n");
	struct alto_db_t * db;
	db = malloc(sizeof(struct alto_db_t));
//	db = (alto_db_t *)malloc(sizeof(struct alto_db_element_t));
	db->first = NULL;
	db->last = NULL;
	db->num_of_elements = 0;
	return db;
}

/*
 *  Kill the DB structure
 */
int alto_free_db(struct alto_db_t * db){
	fprintf(stdout, "Clean and free ALTO database (%p)! \n", db);
	if(db == NULL){
		printf("No DB to be killed! ABORT \n");
		return -1;
	}

	// Now kill every single element
	struct alto_db_element_t * cur;
	cur = db->first;
	while(cur != NULL){
		alto_rem_element(cur);
		cur = cur->next;
	}

	// Free the DB struct & Goodby!
	free(db);
	return 1;
}



/*
 * 	Helper function to print values of one ALTO DB element
 */
void alto_dump_element(struct alto_db_element_t * cur){
	// Sanity check
	if(cur == NULL){
		fprintf(stderr, "No element to print values from! ABORT \n");
	}
	// normal case, print everything
//	fprintf(stdout, "---> Internal Data\t");
//	fprintf(stdout, "Element ptr  : %p \t", cur);
//	fprintf(stdout, "DB   pointer : %p \t", cur->alto_db);
//	fprintf(stdout, "next pointer : %p \t", cur->next);
//	fprintf(stdout, "prev pointer : %p \t", cur->prev);
	fprintf(stdout, "---> User Data\t");
	fprintf(stdout, "host  : %s \t", inet_ntoa(cur->host));
	fprintf(stdout, "prefix: %d \t", cur->host_mask);
	fprintf(stdout, "rating: %d \t", cur->rating);
//	fprintf(stdout, "---> Additional Data\n");
//	fprintf(stdout, "Subnet       : %s \t", inet_ntoa(cur->subnet));
//	fprintf(stdout, "bit mask     : %d \t", cur->subnet_mask);
	fprintf(stdout, "\n");
	return;
}

/*
 * 	Helper function to print values of one ALTO DB element
 */
void alto_dump_db(struct alto_db_t *db){
	fprintf(stdout, "Dump what's in the DB \n");

	// Security Check
	if(db == NULL){
		fprintf(stderr, "Failure in DB structure! ABORT \n");
		return;
	}

	// General Data
	fprintf(stdout,"Number of elements in DB: %d \n", db->num_of_elements);
	fprintf(stdout,"DB->first: %p \n", db->first);
	fprintf(stdout,"DB->last:  %p \n", db->last);

	// List the elements
    struct alto_db_element_t * cur;
    cur = db->first;
    while(cur != NULL){
    	alto_dump_element(cur);
    	cur = cur->next;
    }

    // Success!?
	return;
}




/*
 *  Adds one ALTO entry to the DB
 */
int alto_add_element(struct alto_db_t * db, struct alto_db_element_t * element){
//	fprintf(stdout, "Add an element to the ALTO DB! \n");

	// Error handling
	if(db == NULL || element == NULL){
		fprintf(stderr, "Error in appending element on the DB! ABORT \n");
		return -1;
	}

	// Special case, first element in DB
	if(db->first == NULL && db->last == NULL){

		// Update DB
		db->first = element;
		db->last = element;
		db->num_of_elements = 1;

		// Update element
		element->next = NULL;
		element->prev = NULL;
		element->alto_db = db;
		element->stamp = time(NULL);

		// And success
		return 1;
	}

	// Normal case, append element at the end of the list
	// Update predecessor
	db->last->next = element;

	// Update element
	element->prev = db->last;
	element->next = NULL;
	element->alto_db = db;
	element->stamp = time(NULL);

	// Update DB
	db->last = element;
	db->num_of_elements++;

	// This should be it
	return 1;
}

int alto_rem_element(struct alto_db_element_t * element){

	// Sanity Check
	if(element == NULL){
		fprintf(stderr, "Error in removing element from DB! ABORT \n");
		return -1;
	}

	// Now get the DB where the element is in
	struct alto_db_t *db;
	db = element->alto_db;

	// Special case, last and only element in DB
	if(db->first == element && db->last == element){
		// update main DB
		db->first = NULL;
		db->last = NULL;
		db->num_of_elements = 0;
		// free element
		free(element);
		// exit
		return 1;
	}

	// Special case, first element in chain
	if(db->first == element && db->last != element){
		// Update successor
		element->next->prev = NULL;
		// Update DB
		db->first = element->next;
		db->num_of_elements--;
		// free element
		free(element);
		// exit
		return 1;
	}

	// Special case, last element in chain
	if(db->first != element && db->last == element){
		// Update predecessor
		element->prev->next = NULL;
		// Update DB
		db->last = element->prev;
		db->num_of_elements--;
		// free element
		free(element);
		// exit
		return 1;
	}


	// normal handling, somwhere in the DB
	else {
		// update predecessor
		element->prev->next = element->next;
		element->next->prev = element->prev;
		// Update DB
		db->num_of_elements--;
		// free element
		free(element);
		// exit
		return 1;
	}

	// Just in case something went wrong
	// what NEVER will happen
	fprintf(stderr, "Error in removing element function!\n");
	return -1;
}

struct in_addr compute_subnet(struct in_addr host, int prefix){
	struct in_addr subn;
	uint32_t match_host = host.s_addr;
	uint32_t match_mask = 0xFFFFFFFF;
	match_mask <<= 32 - prefix;
	match_mask = ntohl(match_mask);
	uint32_t match_merg = match_host & match_mask;
//	printf("host  : %s/%d \t", inet_ntoa(host), prefix);
//	subn.s_addr = match_mask;
//	printf("mask  : %s  \t", inet_ntoa(subn));
	subn.s_addr = match_merg;
//	printf("net   : %s  \n", inet_ntoa(subn));
	return subn;
}



/*
 * 	Search in an ALTO DB for the match of an host
 *
 * 	value:	add		Address of the host to search for
 * 			db		The ALTO DB to search in
 * 	return:	value/0	The found value / 0 in case of nothing
 */
int get_ALTO_rating_for_host(struct in_addr add, ALTO_DB_T * db){

	// Sanity chekc I
	if(add.s_addr == 0){
		fprintf(stderr, "Couldn't read the ALTO host IP to query! ABORT\n");
		return 0;
	}

	// Sanity Check II
	if(db == NULL){
		fprintf(stderr, "Couldn't access the DB! ABORT\n");
		return 0;
	}

	// walk through the DB until you find the element
    struct alto_db_element_t * cur;
    cur = db->first;
    while(cur != NULL){
    	if(cur->host.s_addr == add.s_addr){
    		return cur->rating;
    	}
    	cur = cur->next;
    }

    // Here you come in case of error/not found!
	return 0;
}









int ask_helper_func(struct in_addr subnet, ALTO_DB_T * db){
	ALTO_DB_ELEMENT_T * cur = db->first;
	while(cur != NULL){
		if(subnet.s_addr == cur->host.s_addr){
//			printf("Subnet : %s \t", inet_ntoa(subnet));
//			printf("rating : %d \n", cur->rating);
			return cur->rating;
		}
		cur = cur->next;
	}
	return 0;
}



int get_alto_rating(ALTO_DB_ELEMENT_T * element, ALTO_DB_T * db){

	int mask = element->host_mask;
	struct in_addr subnet = compute_subnet(element->host, mask);
//	printf("Host: %s/%d \n", inet_ntoa(element->host), mask);

	int i, check;
	for(i=mask;i>0;i--){
		check = ask_helper_func(subnet, db);
		if(check == 0){
			subnet = compute_subnet(element->host, mask--);
		}else{
//			printf("Found rating, with value: %d \n", check);
			return check;
		}
	}
	return 0;
}



int get_alto_subnet_mask(ALTO_DB_ELEMENT_T * element, ALTO_DB_T * db){
	int mask = element->host_mask;
	struct in_addr subnet = compute_subnet(element->host, mask);
//	printf("Host: %s/%d \n", inet_ntoa(element->host), mask);
	int i;
	for(i=mask;i>0;i--){
		if((ask_helper_func(subnet, db)) == 0){
			return mask;
		}else{
			mask--;
		}
	}
	return 0;
}








/*
 * 	Here the matching between the requested IPs and the delivered
 * 	list will be done
 *
 * 	in:		db_req		Pointer to the DB where the IPs are
 * 			db_res		Pointer to the DB where the ALTO response is
 * 	return:	1			Success
 */
int alto_do_the_magic(ALTO_DB_T * ALTO_db_req, ALTO_DB_T * ALTO_db_res){
	fprintf(stdout, "ALTO do the magic!!! Find the rating and assign to IPs \n");

	// Sanity check
	if(ALTO_db_req == NULL || ALTO_db_res == NULL){
		fprintf(stderr, "Errors in accessing the DBs! ABORT \n");
		return 0;
	}

	// Now iterate through the hosts and find the corresponding rating
	ALTO_DB_ELEMENT_T * cur;
	cur = ALTO_db_req->first;
	while(cur != NULL){

		// store the rating in the asking element
		cur->rating = get_alto_rating(cur, ALTO_db_res);

		// next DB item to query
		cur = cur->next;
	}

	// Aaaaaand finished!
	return 1;
}






int alto_parse_from_file(altoDbPtr db, char *file_name){
	fprintf(stdout, "Read hosts from file (%s) and store it in the Request-DB\n", file_name);

	FILE *file;
	char line[256];
	file = fopen(file_name, "r");
	char * ptr;

	// Sanity checks
	if(db == NULL){
		fprintf(stderr, "No DB select! ABORT \n");
		return 0;
	}

	if(file == NULL){
		fprintf(stderr, "Can't open the file! ABORT \n");
		return 0;
	}

	// Now read the lines in
	while(fgets(line, sizeof(line), file) != NULL ) {
		// parse the line, remove the comments
		ptr = strtok(line, " ");
		// create the ALTO element
		struct alto_db_element_t *element;
		element = malloc(sizeof(ALTO_DB_ELEMENT_T));
		element->host = get_ALTO_host_IP(ptr);
		element->host_mask = get_ALTO_host_mask(ptr);
		// and add this element to the db
		alto_add_element(db, element);
	    }
	return 1;
}



int alto_parse_from_XML(altoDbPtr db, xmlDocPtr doc){
	fprintf(stdout, "Parse an XML element into the DB structure\n");

	// Sanity check
	if(db == NULL || doc == NULL){
		fprintf(stderr, "couldn't access XML or DB! ABORT \n");
		return -1;
	}

	xmlNode *cur = NULL;
	cur = xmlDocGetRootElement(doc);
	xmlChar *overall_rating, *ipprefix = NULL;

	while(cur!=NULL){
		if(!xmlStrcmp(cur->name, BAD_CAST "cnd_hla")){
			overall_rating = xmlGetProp(cur, BAD_CAST "overall_rating");
		}if(!xmlStrcmp(cur->name, BAD_CAST "ipprefix")){
			ipprefix = xmlGetProp(cur, BAD_CAST "prefix");

			// create the ALTO element
			struct alto_db_element_t *element;
			element = malloc(sizeof(ALTO_DB_ELEMENT_T));
			element->host = get_ALTO_host_IP((char*) ipprefix);
			element->host_mask = get_ALTO_host_mask((char*) ipprefix);
			element->rating = atoi(overall_rating);

			// and add this element to the db
			alto_add_element(db, element);

		} if(cur->children != NULL){
//			printf("cur->children == NULL \n");
			cur = cur->children;
		} if(cur->children == NULL && cur->next != NULL){
//			printf("cur->children == NULL && cur->next != NULL \n");
			cur = cur->next;
		} if(cur->children == NULL && cur->next == NULL){
//			printf("cur->children == NULL && cur->next == NULL \n");
			cur = cur->parent->next;
		}
	}

	return 1;
}



/*
 * 	Converts a given alto_list_t structure into the internal DB structure
 *
 * 	in:		db		the databse where the element has to be added
 * 			list	the list of the alto elements structure
 * 			num		number of eements to add
 * 	ret:	1/-1	for success failuire
 */
int alto_parse_from_list(struct alto_db_t *db, struct alto_guidance_t *list, int num_of_elements){
	fprintf(stdout, "Convert given list into internal DB structure\n");

	// Sanity checks
	if(db == NULL || list == NULL){
		fprintf(stderr, "No lists to parse from / parse in, ABORT! \n");
		return -1;
	}

	// and now will the elements in the DB
	int i;
	for(i=0; i < num_of_elements; i++){

		// Create a new element
		struct alto_db_element_t *element;
		element = malloc(sizeof(ALTO_DB_ELEMENT_T));
		element->alto_db = db;
		element->next = NULL;
		element->prev = NULL;
		element->host = list[i].alto_host;			// here the values come in
		element->host_mask = list[i].prefix;		// here the values come in
		element->rating = 0;						// here the values come in

		// and now append it to the DB
		alto_add_element(db, element);
	}

	// That's should be it
	return 1;
}





int alto_write_to_file(altoDbPtr db, char *file_name){
	fprintf(stdout, "Write hosts to file (%s.out) \n", file_name);

	// Sanity Check
	if(file_name == NULL){
		fprintf(stderr, "Can't open the file! ABORT \n");
		return -1;
	}

	// Sanity Check DB
	if(db == NULL){
		fprintf(stderr, "No DB select! ABORT \n");
		return -1;
	}

	// Create the new file
	char * str_out;
	str_out = (char*)malloc(256 * sizeof(char));
	strcpy (str_out,file_name);
	strcat (str_out,".out");

	// create the output filename
	FILE *file;
	file = fopen(str_out, "a+");
	if(file == NULL){
		fprintf(stderr, "Can't create the file! ABORT \n");
		return 0;
	}

	// Write everything to the file
	int count = 0;
	ALTO_DB_ELEMENT_T * cur = db->first;
	while(cur != NULL){
		fprintf(file, "%s/%d \t", inet_ntoa(cur->host), cur->host_mask);
		fprintf(file, "%d \t", cur->rating);
		fprintf(file, "\n");
		count++;
		cur = cur->next;
	}

	// Return the number of sucessful written lines
	return count;
}




/*
 * 	Start & Innitialize the ALTO client
 */
void start_ALTO_client(){
	fprintf(stdout, "START ALTO client! \n");

	// initialize the DBs
	ALTO_DB_req = alto_db_init();
	ALTO_DB_res = alto_db_init();

	// prepare the XML environment
	LIBXML_TEST_VERSION;

	// and Initialize the XMLs
    ALTO_XML_req = NULL;
    ALTO_XML_res = NULL;


}


/*
 * 	Stop & CleanUp the ALTO client
 */
void stop_ALTO_client(){
	fprintf(stdout, "STOP ALTO client! \n");

	// Kill the DBs
	alto_free_db(ALTO_DB_req);
	alto_free_db(ALTO_DB_res);

	// Kill the XML
    xmlFreeDoc(ALTO_XML_req);
    xmlFreeDoc(ALTO_XML_res);

}






/*
 * 	Function:	gets for a list in a txt file the correct rating
 *
 * 	in:			*txt	pointer to the list
 * 				rc_host	The Resource Consumer locator
 * 				pri_rat	Primary Rating criteria
 * 				sec_rat	Secondary Rating criteria
 * 	return:		1/0		Success/Erros
 */
int get_ALTO_guidance_for_txt(char * txt, struct in_addr rc_host, int pri_rat, int sec_rat){
	fprintf(stdout, "get_ALTO_guidance_txt(%s) was called \n", txt);

	// Sanity checks 1
	if(txt == NULL){
		fprintf(stderr, "Can't access the file! ABORT!\n");
		return 0;
	}

	// Sanity checks 1
	if(rc_host.s_addr == NULL){
			fprintf(stderr, "Can't read the rc_host! ABORT!\n");
			return 0;
		}

	// Sanity check 2
	if(pri_rat < 1 || pri_rat > 3){
		fprintf(stderr, "Primary Rating Criteria wrong! ABORT\n");
		return 0;
	}

	if(sec_rat < 1 || sec_rat > 8){
		fprintf(stderr, "Secondary Rating Criteria(s) wrong! ABORT\n");
		return 0;
	}


	// Step 1: fill the txt into the DB
	alto_parse_from_file(ALTO_DB_req, txt);

	// do the ALTO trick / update db
	do_ALTO_update(rc_host, pri_rat, sec_rat);

	// Step 3 (write values back)
	alto_write_to_file(ALTO_DB_req, txt);

	// done
	return 1;
}





/*
 * 	Function:	gets for a list of elements the correct rating
 *
 * 	in:			*list	pointer to the list
 * 				*num	Number of elements to process
 * 	return:		count	Number of sucessfully processed hosts
 */
int get_ALTO_guidance_for_list(ALTO_GUIDANCE_T * list, int num, struct in_addr rc_host, int pri_rat, int sec_rat){
	fprintf(stdout, "get_ALTO_guidance(list, num_of_elements) was called \n");

	int count = 0;

	// Sanity checks (list)
	if(list == NULL){
		fprintf(stderr, "Can't access the list! ABORT!\n");
		return 0;
	}

	// Sanity checks (num of elements)
	if(num < 0){
		fprintf(stderr, "<0 elements? Can't be! ABORT!\n");
		return 0;
	}

	// Step 1 (read struct into DB)
	alto_parse_from_list(ALTO_DB_req, list, num);

	// do the ALTO trick / update db
	do_ALTO_update(rc_host, pri_rat, sec_rat);

	// Step 2 (write values back)
	for(count = 0; count < num; count++){
		list[count].rating = get_ALTO_rating_for_host(list[count].alto_host, ALTO_DB_req);
	}

	// This should be it
	return 1;
}


/*
 * Function:	With this call the internal request to update the DB is triggered.
 * 				This should be done on a regual basis to keep the local ALTO-DB
 * 				up2date
 */


void do_ALTO_update(struct in_addr rc_host, int pri_rat, int sec_rat){

	// Step 2: create an XML from the DB entries
	ALTO_XML_req = alto_create_request_XML(ALTO_DB_req, rc_host, pri_rat, sec_rat);

#ifndef USE_LOCAL_REPLY_XML
	// Step2a: use CURL
	//ALTO_XML_res = query_ALTO_server_curl(ALTO_XML_req, alto_server_url);
	ALTO_XML_res = ALTO_request_to_server(ALTO_XML_req, alto_server_url);
#else
	// Step2b: use for testing the local stored TXT-file
	ALTO_XML_res = xmlReadFile("reply.xml",NULL,XML_PARSE_RECOVER);
#endif

	// Step 3: Parse the XML to the DB
	alto_parse_from_XML(ALTO_DB_res, ALTO_XML_res);

	// ###### Big Magic ######
	// And now check for the corresponding rating
	alto_do_the_magic(ALTO_DB_req, ALTO_DB_res);

}









