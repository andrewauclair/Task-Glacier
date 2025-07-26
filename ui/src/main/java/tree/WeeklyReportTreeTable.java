package tree;

import net.byteseek.swing.treetable.TreeTableHeaderRenderer;
import net.byteseek.swing.treetable.TreeTableModel;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.MutableTreeNode;
import javax.swing.tree.TreeNode;

public class WeeklyReportTreeTable extends JTable {
    public static class WeeklyCategoryNode extends DailyReportTreeTableModel.CategoryNode {
        Long[] minutesPerDay = new Long[7];
        int[] childrenPerDay = new int[7];

        @Override
        public void add(MutableTreeNode child) {
            super.add(child);

            for (int i = 0; i < 7; i++) {
                WeeklyTaskNode taskNode = (WeeklyTaskNode) child;

                if (taskNode.minutesPerDay[i] != null) {
                    minutesPerDay[i] += taskNode.minutesPerDay[i];
                    childrenPerDay[i]++;
                }
            }
        }

        @Override
        public void remove(MutableTreeNode node) {
            super.remove(node);

            for (int i = 0; i < 7; i++) {
                WeeklyTaskNode taskNode = (WeeklyTaskNode) node;

                childrenPerDay[i]--;

                if (childrenPerDay[i] <= 0) {
                    minutesPerDay[i] = null;
                }
                else if (taskNode.minutesPerDay[i] != null) {
                    minutesPerDay[i] -= taskNode.minutesPerDay[i];
                }
            }
        }
    }

    public static class WeeklyTaskNode extends DailyReportTreeTableModel.TaskNode {
        Long[] minutesPerDay = new Long[7];
    }

    private MainFrame mainFrame;
    private DefaultMutableTreeNode rootNode;
    private TreeTableModel treeTableModel;
    private DefaultTreeModel treeModel;


    private DefaultTreeModel createTreeModel(TreeNode rootNode) {
        DefaultTreeModel model = new DefaultTreeModel(rootNode);
        model.addTreeModelListener(treeTableModel);
        return model;
    }

    private TreeTableModel createTreeTableModel(TreeNode rootNode) {
        TreeTableModel localTreeTableModel = new WeeklyReportTreeTableModel(rootNode, null);

        TreeTableHeaderRenderer renderer = new TreeTableHeaderRenderer();
//        renderer.setShowNumber(true); // true is default, this code is just for testing the false option.

        localTreeTableModel.bindTable(this, renderer);
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
}
