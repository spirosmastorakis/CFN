from gi.repository import Gtk

import ns.core
import ns.network
import ns.internet
from ns.ndnSIM import ndn

from visualizer.base import InformationWindow

class ShowNdnFaces(InformationWindow):
    COLUMN_FACE_ID = 0
    COLUMN_FACE = 1

    def __init__(self, visualizer, node_index):
        InformationWindow.__init__(self)
        self.win = Gtk.Dialog(parent=visualizer.window,
                              flags=Gtk.DialogFlags.DESTROY_WITH_PARENT,
                              buttons=(Gtk.STOCK_CLOSE, Gtk.ResponseType.CLOSE))
        self.win.connect("response", self._response_cb)

        self.node = ns.network.NodeList.GetNode (node_index)
        node_name = ns.core.Names.FindName (self.node)

        title = "Ndn FIB for node %i" % node_index
        if len(node_name) != 0:
            title += " (" + str(node_name) + ")"

        self.win.set_title (title)
        self.visualizer = visualizer
        self.node_index = node_index

        self.table_model = Gtk.ListStore(str, str, int)

        treeview = Gtk.TreeView(self.table_model)
        treeview.show()
        sw = Gtk.ScrolledWindow()
        sw.set_properties(hscrollbar_policy=Gtk.PolicyType.AUTOMATIC,
                          vscrollbar_policy=Gtk.PolicyType.AUTOMATIC,
                          hexpand=True, vexpand=True)
        sw.show()
        sw.add(treeview)
        self.win.vbox.add(sw)
        self.win.set_default_size(600, 300)

        # Interface
        column = Gtk.TreeViewColumn('id', Gtk.CellRendererText(),
                                    text=self.COLUMN_FACE_ID)
        treeview.append_column(column)

        # Interface
        column = Gtk.TreeViewColumn('faceType[nodeId]', Gtk.CellRendererText(),
                                    text=self.COLUMN_FACE)
        treeview.append_column(column)

        self.visualizer.add_information_window(self)
        self.win.show()

    def _response_cb(self, win, response):
        self.win.destroy()
        self.visualizer.remove_information_window(self)

    def update(self):
        faces = ndn.L3Protocol.getL3Protocol(self.node).getForwarder().getFaceTable()

        self.table_model.clear()

        for item in faces:
            tree_iter = self.table_model.append()
            self.table_model.set(tree_iter,
                                 self.COLUMN_FACE_ID, str(item.getId()),
                                 self.COLUMN_FACE, str(item))

def populate_node_menu(viz, node, menu):
    menu_item = Gtk.MenuItem("Show NDN Faces")
    menu_item.show()

    def _show_ndn_faces(dummy_menu_item):
        ShowNdnFaces(viz, node.node_index)

    menu_item.connect("activate", _show_ndn_faces)
    menu.add(menu_item)

def register(viz):
    viz.connect("populate-node-menu", populate_node_menu)
