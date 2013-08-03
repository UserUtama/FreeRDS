/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2012
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * main rdp process
 */

#include "xrdp.h"

static int g_session_id = 0;

#include <winpr/crt.h>
#include <winpr/file.h>
#include <winpr/path.h>
#include <winpr/synch.h>
#include <winpr/thread.h>

#include <freerdp/freerdp.h>
#include <freerdp/listener.h>

#include <errno.h>
#include <sys/select.h>
#include <sys/signal.h>

#include "makecert.h"

struct xrdp_process
{
	rdpContext context;

	int status;
	int session_id;
	HANDLE DoneEvent;
	HANDLE TermEvent;

	xrdpWm* wm;
	xrdpSession* session;
};

void xrdp_peer_context_new(freerdp_peer* client, xrdpProcess* context)
{
	rdpSettings* settings = client->settings;

	settings->ColorDepth = 32;
	settings->RemoteFxCodec = TRUE;
	settings->BitmapCacheV3Enabled = TRUE;
	settings->FrameMarkerCommandEnabled = TRUE;

	context->session = libxrdp_session_new(settings);

	if (context->session)
	{
		context->session->context = (rdpContext*) context;
		context->session->client = client;
	}

	context->status = 1;
}

void xrdp_peer_context_free(freerdp_peer* client, xrdpProcess* context)
{

}

xrdpProcess* xrdp_process_create_ex(xrdpListener* owner, HANDLE DoneEvent, void* transport)
{
	xrdpProcess* xfp;
	freerdp_peer* client;

	client = (freerdp_peer*) transport;

	client->ContextSize = sizeof(xrdpProcess);
	client->ContextNew = (psPeerContextNew) xrdp_peer_context_new;
	client->ContextFree = (psPeerContextFree) xrdp_peer_context_free;
	freerdp_peer_context_new(client);

	xfp = (xrdpProcess*) client->context;

	xfp->DoneEvent = DoneEvent;

	g_session_id++;
	xfp->session_id = g_session_id;

	xfp->TermEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	return xfp;
}

void xrdp_process_delete(xrdpProcess* self)
{

}

int xrdp_process_get_status(xrdpProcess* self)
{
	return self->status;
}

HANDLE xrdp_process_get_term_event(xrdpProcess* self)
{
	return self->TermEvent;
}

xrdpSession* xrdp_process_get_session(xrdpProcess* self)
{
	return self->session;
}

int xrdp_process_get_session_id(xrdpProcess* self)
{
	return self->session_id;
}

xrdpWm* xrdp_process_get_wm(xrdpProcess* self)
{
	return self->wm;
}

BOOL xrdp_peer_capabilities(freerdp_peer* client)
{
	return TRUE;
}

BOOL xrdp_peer_post_connect(freerdp_peer* client)
{
	xrdpProcess* xfp;

	xfp = (xrdpProcess*) client->context;

	fprintf(stderr, "Client %s is connected", client->hostname);

	if (client->settings->AutoLogonEnabled)
	{
		fprintf(stderr, " and wants to login automatically as %s\\%s",
			client->settings->Domain ? client->settings->Domain : "",
			client->settings->Username);
	}
	fprintf(stderr, "\n");

	fprintf(stderr, "Client requested desktop: %dx%dx%d\n",
		client->settings->DesktopWidth, client->settings->DesktopHeight, client->settings->ColorDepth);

	/* do not reactivate, just accept client desktop size and color depth */

	return TRUE;
}

BOOL xrdp_peer_activate(freerdp_peer* client)
{
	rdpSettings* settings;
	xrdpProcess* xfp = (xrdpProcess*) client->context;

	settings = client->settings;
	settings->BitmapCacheVersion = 2;

	if (settings->Password)
		settings->AutoLogonEnabled = 1;

	if (settings->RemoteFxCodec || settings->NSCodec)
		xfp->session->codecMode = TRUE;

	if (!xfp->wm)
		xfp->wm = xrdp_wm_create(xfp);

	printf("Client Activated\n");

	return TRUE;
}

const char* makecert_argv[4] =
{
	"makecert",
	"-rdp",
	"-live",
	"-silent"
};

int makecert_argc = (sizeof(makecert_argv) / sizeof(char*));

int xrdp_generate_certificate(rdpSettings* settings)
{
	char* config_home;
	char* server_file_path;
	MAKECERT_CONTEXT* context;

	config_home = GetKnownPath(KNOWN_PATH_XDG_CONFIG_HOME);

	if (!PathFileExistsA(config_home))
		CreateDirectoryA(config_home, 0);

	free(config_home);

	if (!PathFileExistsA(settings->ConfigPath))
		CreateDirectoryA(settings->ConfigPath, 0);

	server_file_path = GetCombinedPath(settings->ConfigPath, "server");

	if (!PathFileExistsA(server_file_path))
		CreateDirectoryA(server_file_path, 0);

	settings->CertificateFile = GetCombinedPath(server_file_path, "server.crt");
	settings->PrivateKeyFile = GetCombinedPath(server_file_path, "server.key");

	if ((!PathFileExistsA(settings->CertificateFile)) ||
			(!PathFileExistsA(settings->PrivateKeyFile)))
	{
		context = makecert_context_new();

		makecert_context_process(context, makecert_argc, (char**) makecert_argv);

		makecert_context_set_output_file_name(context, "server");

		if (!PathFileExistsA(settings->CertificateFile))
			makecert_context_output_certificate_file(context, server_file_path);

		if (!PathFileExistsA(settings->PrivateKeyFile))
			makecert_context_output_private_key_file(context, server_file_path);

		makecert_context_free(context);
	}

	free(server_file_path);

	return 0;
}

void xrdp_input_synchronize_event(rdpInput* input, UINT32 flags)
{

}

void xrdp_input_keyboard_event(rdpInput* input, UINT16 flags, UINT16 code)
{
	xrdpProcess* xfp = (xrdpProcess*) input->context;

	if (xfp->wm)
		xrdp_wm_key(xfp->wm, flags, code);
}

void xrdp_input_unicode_keyboard_event(rdpInput* input, UINT16 flags, UINT16 code)
{

}

void xrdp_input_mouse_event(rdpInput* input, UINT16 flags, UINT16 x, UINT16 y)
{
	xrdpProcess* xfp = (xrdpProcess*) input->context;

	if (xfp->wm)
		xrdp_wm_process_input_mouse(xfp->wm, flags, x, y);
}

void xrdp_input_extended_mouse_event(rdpInput* input, UINT16 flags, UINT16 x, UINT16 y)
{
	xrdpProcess* xfp = (xrdpProcess*) input->context;

	if (xfp->wm)
		xrdp_wm_process_input_mouse(xfp->wm, flags, x, y);
}

void xrdp_input_register_callbacks(rdpInput* input)
{
	input->SynchronizeEvent = xrdp_input_synchronize_event;
	input->KeyboardEvent = xrdp_input_keyboard_event;
	input->UnicodeKeyboardEvent = xrdp_input_unicode_keyboard_event;
	input->MouseEvent = xrdp_input_mouse_event;
	input->ExtendedMouseEvent = xrdp_input_extended_mouse_event;
}

void xrdp_update_frame_acknowledge(rdpContext* context, UINT32 frameId)
{
	printf("FrameAck: %d\n", frameId);
}

void* xrdp_process_main_thread(void* arg)
{
	DWORD status;
	DWORD nCount;
	HANDLE events[32];
	xrdpProcess* xfp;
	HANDLE ClientEvent;
	HANDLE LocalTermEvent;
	HANDLE GlobalTermEvent;
	rdpSettings* settings;
	freerdp_peer* client = (freerdp_peer*) arg;

	fprintf(stderr, "We've got a client %s\n", client->hostname);

	xfp = (xrdpProcess*) client->context;
	settings = client->settings;

	xrdp_generate_certificate(settings);

	settings->RdpSecurity = FALSE;
	settings->TlsSecurity = TRUE;
	settings->NlaSecurity = FALSE;

	client->Capabilities = xrdp_peer_capabilities;
	client->PostConnect = xrdp_peer_post_connect;
	client->Activate = xrdp_peer_activate;

	client->Initialize(client);

	xfp->session->callback = callback;
	xrdp_input_register_callbacks(client->input);

	client->update->SurfaceFrameAcknowledge = xrdp_update_frame_acknowledge;

	ClientEvent = client->GetEventHandle(client);
	GlobalTermEvent = g_get_term_event();
	LocalTermEvent = xfp->TermEvent;

	while (1)
	{
		nCount = 0;
		events[nCount++] = ClientEvent;
		events[nCount++] = GlobalTermEvent;
		events[nCount++] = LocalTermEvent;

		if (client->activated)
		{
			xrdp_wm_get_event_handles(xfp->wm, events, &nCount);
		}

		status = WaitForMultipleObjects(nCount, events, FALSE, INFINITE);

		if (WaitForSingleObject(GlobalTermEvent, 0) == WAIT_OBJECT_0)
		{
			break;
		}

		if (WaitForSingleObject(LocalTermEvent, 0) == WAIT_OBJECT_0)
		{
			break;
		}

		if (WaitForSingleObject(ClientEvent, 0) == WAIT_OBJECT_0)
		{
			if (client->CheckFileDescriptor(client) != TRUE)
			{
				fprintf(stderr, "Failed to check freerdp file descriptor\n");
				break;
			}
		}

		if (client->activated)
		{
			if (xrdp_wm_check_wait_objs(xfp->wm) != 0)
			{
				break;
			}
		}
	}

	fprintf(stderr, "Client %s disconnected.\n", client->hostname);

	client->Disconnect(client);

	freerdp_peer_context_free(client);
	freerdp_peer_free(client);

	return NULL;
}

