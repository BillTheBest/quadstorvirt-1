/* 
 * Copyright (C) Shivaram Upadhyayula <shivaram.u@quadstor.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301, USA.
 */

#include "cgimain.h"

int main()
{
	llist entries;
	uint32_t target_id;
	int retval;
	struct iscsiconf iscsiconf;
	char *tmp;
	char cmd[512], reply[256];
	char *IncomingUser, *IncomingPasswd, *OutgoingUser, *OutgoingPasswd, *iqn;

	read_cgi_input(&entries);

	tmp = cgi_val(entries, "target_id");
	if (!tmp)
		cgi_print_header_error_page("Invalid CGI Parameters\n");

	target_id = atoi(tmp);
	if (!target_id)
		cgi_print_header_error_page("Invalid CGI Parameters\n");

	memset(&iscsiconf, 0, sizeof(struct iscsiconf));

	iscsiconf.target_id = target_id;

	iqn = cgi_val(entries, "iqn");
	if (!iqn || strlen(iqn) > 255)
		cgi_print_header_error_page("Invalid CGI Parameters\n");

	IncomingUser = cgi_val(entries, "IncomingUser");
	if (!IncomingUser || strlen(IncomingUser) > 36)
		cgi_print_header_error_page("Invalid CGI Parameters\n");

	IncomingPasswd = cgi_val(entries, "IncomingPasswd");
	if (!IncomingPasswd || strlen(IncomingPasswd) > 36)
		cgi_print_header_error_page("Invalid CGI Parameters\n");

	OutgoingUser = cgi_val(entries, "OutgoingUser");
	if (!OutgoingUser || strlen(OutgoingUser) > 36)
		cgi_print_header_error_page("Invalid CGI Parameters\n");

	OutgoingPasswd = cgi_val(entries, "OutgoingPasswd");
	if (!OutgoingPasswd || strlen(OutgoingPasswd) > 36)
		cgi_print_header_error_page("Invalid CGI Parameters\n");

	if (strlen(IncomingUser) && !strlen(IncomingPasswd))
		cgi_print_header_error_page("Invalid Incoming Passwd");

	if (!strlen(IncomingUser))
		IncomingPasswd ="";
		
	if (strlen(OutgoingUser) && !strlen(OutgoingPasswd))
		cgi_print_header_error_page("Invalid Outgoing Passwd");

	if (!strlen(OutgoingUser))
		OutgoingPasswd ="";

	strcpy(iscsiconf.iqn, iqn);
	strcpy(iscsiconf.IncomingUser, IncomingUser);
	strcpy(iscsiconf.IncomingPasswd, IncomingPasswd);
	strcpy(iscsiconf.OutgoingUser, OutgoingUser);
	strcpy(iscsiconf.OutgoingPasswd, OutgoingPasswd);

	retval = tl_client_set_iscsiconf(&iscsiconf, reply);
	if (retval != 0) {
		sprintf (cmd, "Setting iSCSI parameters failed<br/>Message from server is:<br/>\"%s\"\n", reply);
		cgi_print_header_error_page(cmd);
	}

	sprintf(cmd, "redirect.cgi?cgiscript=modifytdisk.cgi&target_id=%u&title=%s&refresh=1&msg=%s", target_id, "Update iSCSI Settings", "Done updating iSCSI settings");
	cgi_redirect(cmd);
	return 0;
}

