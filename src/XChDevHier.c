/************************************************************

Copyright 2007 Peter Hutterer <peter@cs.unisa.edu.au>

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/

/***********************************************************************
 *
 * XIChangeDeviceHierarchy - change the device hierarchy, i.e. which slave
 * device is attached to which master, etc.
 */

#include <stdint.h>
#include <X11/extensions/XI.h>
#include <X11/extensions/XI2proto.h>
#include <X11/Xlibint.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/extutil.h>
#include "XIint.h"

int
XIChangeDeviceHierarchy(Display* dpy,
                        XIAnyHierarchyChangeInfo* changes,
                        int num_changes)
{
    XIAnyHierarchyChangeInfo* any;
    xXIChangeDeviceHierarchyReq *req;
    XExtDisplayInfo *info = XInput_find_display(dpy);
    char *data = NULL, *dptr;
    int dlen = 0, i;

    LockDisplay(dpy);
    if (_XiCheckExtInit(dpy, XInput_2, info) == -1)
	return (NoSuchExtension);

    GetReq(XIChangeDeviceHierarchy, req);
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_XIChangeDeviceHierarchy;
    req->num_changes = num_changes;

    /* alloc required memory */
    for (i = 0, any = changes; i < num_changes; i++, any++)
    {
        switch(any->type)
        {
            case CH_CreateMasterDevice:
                {
                    int slen = (strlen(any->create.name));
                    dlen += sizeof(xXICreateMasterInfo) +
                        slen + (4 - (slen % 4));
                }
                break;
            case CH_RemoveMasterDevice:
                dlen += sizeof(xXIRemoveMasterInfo);
                break;
            case CH_AttachSlave:
                dlen += sizeof(xXIAttachSlaveInfo);
                break;
            case CH_DetachSlave:
                dlen += sizeof(xXIDetachSlaveInfo);
                break;
            default:
                return BadValue;
        }
    }

    req->length += dlen / 4; /* dlen is 4-byte aligned */
    data = Xmalloc(dlen);
    if (!data)
        return BadAlloc;

    dptr = data;
    for (i = 0, any = changes; i < num_changes; i++, any++)
    {
        switch(any->type)
        {
                case CH_CreateMasterDevice:
                {
                    XICreateMasterInfo* C = &any->create;
                    xXICreateMasterInfo* c = (xXICreateMasterInfo*)dptr;
                    c->type = C->type;
                    c->send_core = C->sendCore;
                    c->enable = C->enable;
                    c->name_len = strlen(C->name);
                    c->length = (sizeof(xXICreateMasterInfo) + c->name_len + 3)/4;
                    strncpy((char*)&c[1], C->name, c->name_len);
                    dptr += c->length;
                }
                break;
            case CH_RemoveMasterDevice:
                {
                    XIRemoveMasterInfo* R = &any->remove;
                    xXIRemoveMasterInfo* r = (xXIRemoveMasterInfo*)dptr;
                    r->type = R->type;
                    r->return_mode = R->returnMode;
                    r->deviceid = R->device;
                    r->length = sizeof(xXIRemoveMasterInfo)/4;
                    if (r->return_mode == AttachToMaster)
                    {
                        r->return_pointer = R->returnPointer;
                        r->return_keyboard = R->returnKeyboard;
                    }
                    dptr += sizeof(xXIRemoveMasterInfo);
                }
                break;
            case CH_AttachSlave:
                {
                    XIAttachSlaveInfo* C = &any->attach;
                    xXIAttachSlaveInfo* c = (xXIAttachSlaveInfo*)dptr;

                    c->type = C->type;
                    c->deviceid = C->device;
                    c->length = sizeof(xXIAttachSlaveInfo)/4;
                    c->new_master = C->newMaster;

                    dptr += sizeof(xXIAttachSlaveInfo);
                }
                break;
            case CH_DetachSlave:
                {
                    XIDetachSlaveInfo *D = &any->detach;
                    xXIDetachSlaveInfo *d = (xXIDetachSlaveInfo*)dptr;

                    d->type = D->type;
                    d->deviceid = D->device;
                    d->length = sizeof(xXIDetachSlaveInfo)/4;
                    dptr += sizeof(xXIDetachSlaveInfo);
                }
        }
    }

    Data(dpy, data, dlen);
    Xfree(data);
    UnlockDisplay(dpy);
    SyncHandle();
    return Success;
}
