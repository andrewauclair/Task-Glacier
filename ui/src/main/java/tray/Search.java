package tray;

import data.Task;
import data.TaskModel;
import data.TaskState;
import net.byteseek.swing.treetable.TreeTableHeaderRenderer;
import net.byteseek.swing.treetable.TreeTableModel;
import net.byteseek.swing.treetable.TreeUtils;
import tree.TaskTreeTableModel;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.MutableTreeNode;
import javax.swing.tree.TreeNode;
import java.awt.*;
import java.util.Enumeration;

class Search extends JPanel implements TaskModel.Listener {
    private final MainFrame mainFrame;

    private JTable table;
    private DefaultMutableTreeNode rootNode;
    private TreeTableModel treeTableModel;
    private DefaultTreeModel treeModel;

    public Search(MainFrame mainFrame) {
        this.mainFrame = mainFrame;
        this.mainFrame.getTaskModel().addListener(this);

        Task rootObject = new Task(0, 0, "");
        rootNode = TreeUtils.buildTree(rootObject, Task::getChildren, parent -> false);

        table = new JTable();
        table.setSelectionMode(ListSelectionModel.MULTIPLE_INTERVAL_SELECTION);
        table.setDragEnabled(false);
//        table.setDropMode(DropMode.ON_OR_INSERT_ROWS);
//        table.setTransferHandler(new AltTasksList.TaskTransferHandler());
        treeTableModel = createTreeTableModel(rootNode);
        treeModel = createTreeModel(rootNode);

        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        add(new JScrollPane(table), gbc);
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

        localTreeTableModel.bindTable(table, renderer); //, new RowSorter.SortKey(0, SortOrder.ASCENDING));
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

    public void setSearchText(final String search) {
        treeTableModel.setNodeFilter(treeNode -> {
            // never filter the root. we keep it hidden
            if (treeNode == rootNode) {
                return false;
            }
            Task obj = TreeUtils.getUserObject(treeNode);
            boolean includeFinish = search.startsWith("finish: ");
            String text = search;
            if (includeFinish) {
                text = text.substring("finish: ".length());
                return !childrenHaveMatch(obj, text);
            }
            return obj.state == TaskState.FINISHED || !childrenHaveMatch(obj, text);
        });
    }

    private boolean childrenHaveMatch(Task obj, String text) {
        for (Task child : obj.children) {
            if (childrenHaveMatch(child, text)) {
                return true;
            }
        }
        if (obj.name.toLowerCase().contains(text.toLowerCase())) {
            return true;
        }
        return false;
    }

    @Override
    public void newTask(Task task) {
        System.out.println("AltTasksList.newTask");
        DefaultMutableTreeNode parent = findTaskNode(rootNode, task.parentID);
        if (parent == null) {
            int breakpoint = 0;
            return;
        }
        MutableTreeNode newChild = new DefaultMutableTreeNode(task);

        parent.setAllowsChildren(true);
        parent.add(newChild);
        treeTableModel.treeNodeInserted(parent, parent.getChildCount() - 1);

        if (task.parentID == 0 && parent.getChildCount() == 1) {
            treeTableModel.expandTree();
        }
    }

    @Override
    public void updatedTask(Task task) {
        System.out.println("AltTasksList.updatedTask");
        DefaultMutableTreeNode node = findTaskNode(rootNode, task.id);

        if (node != null) {
            treeTableModel.treeNodeChanged(node);

            if (task.state == TaskState.FINISHED) {
//                updateFilter();

                Task parent = mainFrame.getTaskModel().getTask(task.parentID);
                DefaultMutableTreeNode parentNode = findTaskNode(rootNode, task.parentID);

                if (parent != null && parentNode != null) {
                    boolean active = parent.children.stream()
                            .anyMatch(task1 -> task1.state != TaskState.FINISHED);

                    parentNode.setAllowsChildren(active);
                }
            }
        }
    }

    @Override
    public void reparentTask(Task task, int oldParent) {
        System.out.println("AltTasksList.reparentTask");
        DefaultMutableTreeNode oldParentNode = findTaskNode(rootNode, oldParent);
        DefaultMutableTreeNode newParentNode = findTaskNode(rootNode, task.parentID);

        DefaultMutableTreeNode node = findTaskNode(rootNode, task.id);

        if (node == null) {
            return;
        }

        if (oldParentNode != null) {
            if (oldParentNode.getChildCount() == 0) {
                oldParentNode.setAllowsChildren(false);
            }
            // also removes the node from the old parent for us
            treeTableModel.treeNodeRemoved(oldParentNode, node);
        }

        if (newParentNode != null) {
            newParentNode.setAllowsChildren(true);
            newParentNode.add(node);
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
