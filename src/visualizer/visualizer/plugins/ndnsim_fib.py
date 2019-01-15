from gi.repository import Gtk

import ns.core
import ns.network
import ns.internet
from ns.ndnSIM import ndn

from visualizer.base import InformationWindow

class ShowNdnFib(InformationWindow):
    (
        COLUMN_PREFIX,
        COLUMN_FACE
        ) = range(2)

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
        
        # Dest.
        column = Gtk.TreeViewColumn('Destination', Gtk.CellRendererText(),
                                    text=self.COLUMN_PREFIX)
        treeview.append_column(column)

        # Interface
        column = Gtk.TreeViewColumn('faceType[nodeId](routingCost,status,metric)', Gtk.CellRendererText(),
                                    text=self.COLUMN_FACE)
        treeview.append_column(column)

        self.visualizer.add_information_window(self)
        self.win.show()

    def _response_cb(self, win, response):
        self.win.destroy()
        self.visualizer.remove_information_window(self)
    
    def update(self):
        ndnFib = ndn.L3Protocol.getL3Protocol(self.node).getForwarder().getFib()

        if ndnFib is None:
            return

        self.table_model.clear()

        for item in ndnFib:
            tree_iter = self.table_model.append()
            self.table_model.set(tree_iter,
                                 self.COLUMN_PREFIX, str(item.getPrefix()),
                                 self.COLUMN_FACE, ", ".join(["%s%d (%d)" % (str(nh.getFace()), nh.getFace().getId(), nh.getCost()) for nh in item.getNextHops()]))

def populate_node_menu(viz, node, menu):
    menu_item = Gtk.MenuItem("Show NDN FIB")
    menu_item.show()

    def _show_ndn_fib(dummy_menu_item):
        ShowNdnFib(viz, node.node_index)

    menu_item.connect("activate", _show_ndn_fib)
    menu.add(menu_item)

def register(viz):
    viz.connect("populate-node-menu", populate_node_menu)
