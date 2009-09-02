#include <wx/frame.h>
#include <wx/socket.h>
#include <wx/listimpl.cpp>
#include <wx/app.h>

#include "../indi_io.h"

class IndiCB {
public:
	int (*cmd)(void *data);
	void *data;
};

WX_DECLARE_LIST(IndiCB, cbList);
WX_DEFINE_LIST(cbList);
static cbList idle;
enum {
	IDLE_ID   = 9998,
	SOCKET_ID = 9999,
};

class IndiIO : public wxEvtHandler
{ 
public:
	IndiIO();
	~IndiIO();

	// socket event handler
	void OnIdleEvent(wxIdleEvent& event);
	// socket event handler
	void OnSocketEvent(wxSocketEvent& event);

	void (*sock_read_cb)(void *fd, void *opaque);
	void * sock_read_cb_data;

	wxSocketClient *m_sock;

};

//static IndiIO *indiIO;

IndiIO::IndiIO() : wxEvtHandler()
{
	//indiIO = this;
	m_sock = new wxSocketClient();
	m_sock->SetEventHandler(*this, SOCKET_ID);
	m_sock->SetNotify(wxSOCKET_CONNECTION_FLAG |
	                  wxSOCKET_INPUT_FLAG |
	                  wxSOCKET_LOST_FLAG);
	m_sock->Notify(true);
	Connect(IDLE_ID, wxEVT_IDLE, wxIdleEventHandler(IndiIO::OnIdleEvent));
	Connect(SOCKET_ID, wxEVT_SOCKET, wxSocketEventHandler(IndiIO::OnSocketEvent));
}

IndiIO::~IndiIO()
{
	delete m_sock;
}

void IndiIO::OnSocketEvent(wxSocketEvent& event)
{ 
	if (event.GetSocketEvent() == wxSOCKET_INPUT) {
		if (sock_read_cb) {
			sock_read_cb(m_sock, sock_read_cb_data);
		}
	}
}

void IndiIO::OnIdleEvent(wxIdleEvent& event)
{
	IndiCB *cb;
	if(idle.IsEmpty())
		return;
	cb = idle.front();
	idle.pop_front();
	if (cb->cmd(cb->data)) {
		idle.push_back(cb);
	} else {
		delete cb;
	}
	if(! idle.IsEmpty())
		event.RequestMore();
}

int io_indi_sock_read(void *fh, void *data, int len)
{
	wxSocketBase *m_sock = (wxSocketBase *)fh;

	m_sock->Read(data, len);
	if (m_sock->Error()) {
		return -1;
	}
	return m_sock->LastCount();
}

int io_indi_sock_write(void *fh, void *data, int len)
{
	wxSocketBase *m_sock = (wxSocketBase *)fh;

	m_sock->Write(data, len);
	if (m_sock->Error()) {
		return -1;
	}
	return m_sock->LastCount();
}

void *io_indi_open_server(const char *host, int port, void (*cb)(void *fd, void *opaque), void *opaque)
{
	bool success;
	wxIPV4address addr;
	wxString hostName(host, wxConvUTF8);
	IndiIO *indiIO = new IndiIO();

	indiIO->sock_read_cb = cb;
	indiIO->sock_read_cb_data = opaque;

	addr.Hostname(hostName);
	addr.Service(port);
	success = indiIO->m_sock->Connect(addr, true);
	if(! success) {
		delete indiIO;
		return NULL;
	}
	return indiIO->m_sock;
}

void io_indi_idle_callback(int (*cb)(void *data), void *data)
{
	IndiCB *indiCB = new IndiCB();
	indiCB->cmd = cb;
	indiCB->data = data;
	idle.push_back(indiCB);
}
