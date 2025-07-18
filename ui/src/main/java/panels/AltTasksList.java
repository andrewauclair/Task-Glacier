package panels;

import data.Task;
import data.TaskModel;
import io.github.andrewauclair.moderndocking.Dockable;
import io.github.andrewauclair.moderndocking.app.Docking;
import net.byteseek.swing.treetable.TreeTableHeaderRenderer;
import net.byteseek.swing.treetable.TreeTableModel;
import net.byteseek.swing.treetable.TreeUtils;
import packets.RequestID;
import packets.UpdateTask;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.tree.*;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.datatransfer.StringSelection;
import java.awt.datatransfer.Transferable;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Enumeration;
import java.util.List;

import static taskglacier.MainFrame.mainFrame;

public class AltTasksList extends JPanel implements Dockable, TaskModel.Listener {
    private final DefaultMutableTreeNode rootNode;
    private TreeTableModel treeTableModel;
    private DefaultTreeModel treeModel;

    private JTable table1;
    public JTextField sTextField = new JTextField(20);

    public AltTasksList(MainFrame mainFrame) {
        Docking.registerDockable(this);
        mainFrame.getTaskModel().addListener(this);


        Task rootObject = new Task(0, 0, "");
        rootNode = TreeUtils.buildTree(rootObject, Task::getChildren, parent -> { return false; });
        table1 = new JTable();
        table1.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        table1.setDragEnabled(true);
        table1.setDropMode(DropMode.ON_OR_INSERT_ROWS);
        table1.setTransferHandler(new TaskTransferHandler());
        treeTableModel = createTreeTableModel(rootNode);
        treeModel = createTreeModel(rootNode);

        sTextField.getDocument().addDocumentListener(new DocumentListener() {
            @Override
            public void insertUpdate(DocumentEvent e) {
                updateFilter();
            }

            @Override
            public void removeUpdate(DocumentEvent e) {
                updateFilter();
            }

            @Override
            public void changedUpdate(DocumentEvent e) {
                updateFilter();
            }

            private void updateFilter() {
                if (sTextField.getText().isEmpty()) {
                    treeTableModel.setNodeFilter(null);
                } else {
                    treeTableModel.setNodeFilter(treeNode -> {
                        Task obj = TreeUtils.getUserObject(treeNode);
                        return !childrenHaveMatch(obj, sTextField.getText());
                    });
                }
            }
        });

        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.fill = GridBagConstraints.HORIZONTAL;

        add(sTextField, gbc);

        gbc.gridy++;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        add(new JScrollPane(table1), gbc);
    }
    class TaskTransferHandler extends TransferHandler {
        private int[] rows;

        @Override
        public int getSourceActions(JComponent c) {
            return MOVE;
        }

        @Override
        protected Transferable createTransferable(JComponent c) {
            rows = table1.getSelectedRows();

            int parent = -1;

            // all the rows must have the same parent
            for (int row : rows) {
//                TreePath pathForRow = table.getPathForRow(row);
//                TaskTreeTableNode node = (TaskTreeTableNode) pathForRow.getLastPathComponent();
//                Task task = (Task) node.getUserObject();
                DefaultMutableTreeNode nodeAtTableRow = (DefaultMutableTreeNode) treeTableModel.getNodeAtTableRow(row);
                Task task = (Task) nodeAtTableRow.getUserObject();

                if (parent == -1) {
                    parent = task.parentID;
                }
                else if (task.parentID != parent) {
//                    return null;
                }
            }

            return new StringSelection("");
        }

        @Override
        public boolean canImport(TransferSupport support) {
            return true;
        }

        @Override
        public boolean importData(TransferSupport support) {
            // send to server
            for (int row : rows) {
                DefaultMutableTreeNode nodeAtTableRow = (DefaultMutableTreeNode) treeTableModel.getNodeAtTableRow(row);
                Task task = (Task) nodeAtTableRow.getUserObject();

                JTable.DropLocation dl = (JTable.DropLocation) support.getDropLocation();

                DefaultMutableTreeNode dropNode = (DefaultMutableTreeNode) treeTableModel.getNodeAtTableRow(dl.getRow());

                // dropping after the last node on the tree
                if (dropNode == null) {
                    System.out.println("insert as last task in root list");

                    
                }
                else {
                    Task dropTask = (Task) dropNode.getUserObject();
                    Task parentTask;

                    // the drop row is always the row below where the insert line is
                    if (dl.isInsertRow()) {
                        parentTask = mainFrame.getTaskModel().getTask(dropTask.parentID);
                    }
                    else {
                        parentTask = dropTask;
                    }
                    int index = dl.isInsertRow() ? dropTask.indexInParent : 0;

                    if (parentTask == null) {
                        System.out.println("insert with root as parent");

                        UpdateTask update = new UpdateTask(RequestID.nextRequestID(), task.id, 0, task.name);
                        update.indexInParent = index;
                        mainFrame.getConnection().sendPacket(update);
                    }
                    else {
                        System.out.println("Reparent '" + task.name + "' to '" + parentTask.name);

                        UpdateTask update = new UpdateTask(RequestID.nextRequestID(), task.id, parentTask.id, task.name);
                        update.indexInParent = index;
                        mainFrame.getConnection().sendPacket(update);
                    }
                    // move task: request id, task id, new parent id

                }
            }
            return true;
        }
    }

    private boolean childrenHaveMatch(Task obj, String text) {
        for (Task child : obj.children) {
            if (childrenHaveMatch(child, text)) {
                return true;
            }
        }
        // if we made it this far, and we have children, return false
        if (!obj.children.isEmpty()) {
            return false;
        }
        return obj.name.contains(text);
    }

    private DefaultTreeModel createTreeModel(TreeNode rootNode) {
        DefaultTreeModel model = new DefaultTreeModel(rootNode);
        model.addTreeModelListener(treeTableModel);
        return model;
    }

    private TreeTableModel createTreeTableModel(TreeNode rootNode) {
        TreeTableModel localTreeTableModel = new TaskTreeTableModel(rootNode, false);

        TreeTableHeaderRenderer renderer = new TreeTableHeaderRenderer();
        renderer.setShowNumber(true); // true is default, this code is just for testing the false option.

        localTreeTableModel.bindTable(table1, renderer); //, new RowSorter.SortKey(0, SortOrder.ASCENDING));
        localTreeTableModel.addExpandCollapseListener(new TreeTableModel.ExpandCollapseListener() {
            @Override
            public boolean nodeExpanding(TreeNode node) {
                if (node.getChildCount() == 0) { // if a node is expanding but has no children, set it to allow no children.
                    ((DefaultMutableTreeNode) node).setAllowsChildren(false);
                }
                return true;
            }

            @Override
            public boolean nodeCollapsing(TreeNode node) {
                return true;
            }
        });
        return localTreeTableModel;
    }

    @Override
    public String getPersistentID() {
        return "alt-tasks-list";
    }

    @Override
    public String getTabText() {
        return "Alt Tasks List";
    }

    @Override
    public void newTask(Task task) {
        DefaultMutableTreeNode parent = findTaskNode(rootNode, task.parentID);
        if (parent == null) {
            int breakpoint = 0;
            return;
        }
        MutableTreeNode newChild = new DefaultMutableTreeNode(task);
        parent.setAllowsChildren(true);
//        int index = parent.getChildCount() - 1;
//        parent.insert(newChild, index);
        parent.add(newChild);
        treeTableModel.treeNodeInserted(parent, parent.getChildCount() - 1);

    }

    @Override
    public void updatedTask(Task task) {
        DefaultMutableTreeNode parent = findTaskNode(rootNode, task.parentID);
        DefaultMutableTreeNode node = findTaskNode(rootNode, task.id);

        if (node != null) {
            treeTableModel.treeNodeChanged(node);
//            if (parent.getIndex(node) != task.indexInParent) {
//                ArrayList<TreeNode> nodes = Collections.list(parent.children());
//                List<Task> list = nodes.stream()
//                        .map(treeNode -> (Task)((DefaultMutableTreeNode) treeNode).getUserObject())
//                        .sorted(Comparator.comparingInt(o -> o.indexInParent))
//                        .toList();
//
//                parent.removeAllChildren();
//
//                for (TreeNode removedNode : nodes) {
//                    treeTableModel.treeNodeRemoved(parent, removedNode);
//                }
//
//                nodes.sort(Comparator.comparingInt(o -> ((Task) ((DefaultMutableTreeNode) o).getUserObject()).indexInParent));
//                for (TreeNode addNode : nodes) {
//                    parent.add((MutableTreeNode) addNode);
//                    treeTableModel.treeNodeInserted(parent, parent.getChildCount() - 1);
//                }
//            }
//            else {
//                treeTableModel.treeNodeChanged(node);
//            }
        }
    }

    @Override
    public void reparentTask(Task task, int oldParent) {
        DefaultMutableTreeNode oldParentNode = findTaskNode(rootNode, oldParent);
        DefaultMutableTreeNode newParentNode = findTaskNode(rootNode, task.parentID);

        DefaultMutableTreeNode node = findTaskNode(rootNode, task.id);

        if (node == null) {
            return;
        }

        if (oldParentNode != null) {
            oldParentNode.remove(node);
            if (oldParentNode.getChildCount() == 0) {
                oldParentNode.setAllowsChildren(false);
            }
            treeTableModel.treeNodeRemoved(oldParentNode, node);
        }

        if (newParentNode != null) {
            newParentNode.setAllowsChildren(true);
            newParentNode.add(node);//, task.indexInParent);
            treeTableModel.treeNodeInserted(newParentNode, newParentNode.getChildCount() - 1);
        }
    }

    @Override
    public void configComplete() {
        treeTableModel.expandTree();
    }

    private DefaultMutableTreeNode findTaskNode(DefaultMutableTreeNode currentParent, int parentID) {
        if (parentID == 0) {
            return rootNode;
        }
        Enumeration<TreeNode> children = currentParent.children();

        while (children.hasMoreElements()) {
            DefaultMutableTreeNode node = (DefaultMutableTreeNode) children.nextElement();

            Task task = (Task) node.getUserObject();

            if (task.id == parentID) {
                return node;
            }

            if (node.getChildCount() != 0) {
                DefaultMutableTreeNode result = findTaskNode(node, parentID);

                if (result != null) {
                    return result;
                }
            }
        }

        return null;
    }
}
