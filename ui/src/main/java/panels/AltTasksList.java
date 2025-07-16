package panels;

import data.Task;
import data.TaskModel;
import io.github.andrewauclair.moderndocking.Dockable;
import io.github.andrewauclair.moderndocking.app.Docking;
import net.byteseek.demo.treetable.MyObject;
import net.byteseek.demo.treetable.MyObjectTreeTableModel;
import net.byteseek.swing.treetable.TreeTableHeaderRenderer;
import net.byteseek.swing.treetable.TreeTableModel;
import net.byteseek.swing.treetable.TreeUtils;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.TreeNode;

import java.awt.*;

import static net.byteseek.demo.treetable.MyObjectForm.CHANCE_OUT_OF_TEN_FOR_CHILDREN;
import static net.byteseek.demo.treetable.MyObjectForm.MAX_LEVELS;

public class AltTasksList extends JPanel implements Dockable, TaskModel.Listener {
    private TreeTableModel treeTableModel;
    private DefaultTreeModel treeModel;

    private JTable table1;
    public JTextField sTextField;

    public AltTasksList(MainFrame mainFrame) {
        Docking.registerDockable(this);
        mainFrame.getTaskModel().addListener(this);


        Task rootObject = new Task(0, 0, "");
        TreeNode rootNode = TreeUtils.buildTree(rootObject, Task::getChildren, parent -> { return false; });
        table1 = new JTable();
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

    private boolean childrenHaveMatch(Task obj, String text) {
        for (Task child : obj.children) {
            if (childrenHaveMatch(child, text)) {
                return true;
            }
        }

        return obj.name.contains(text);
    }

    private DefaultTreeModel createTreeModel(TreeNode rootNode) {
        DefaultTreeModel model = new DefaultTreeModel(rootNode);
        model.addTreeModelListener(treeTableModel);
        return model;
    }

    private TreeTableModel createTreeTableModel(TreeNode rootNode) {
        TreeTableModel localTreeTableModel = new MyObjectTreeTableModel(rootNode, false);

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
    }

    @Override
    public void updatedTask(Task task, boolean parentChanged) {

    }
}
