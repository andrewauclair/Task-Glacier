package tree;

import data.Task;
import data.TimeData;
import net.byteseek.swing.treetable.TableUtils;
import net.byteseek.swing.treetable.TreeTableModel;
import net.byteseek.swing.treetable.TreeUtils;
import org.jdesktop.swingx.treetable.DefaultMutableTreeTableNode;
import org.jdesktop.swingx.treetable.MutableTreeTableNode;

import javax.swing.table.DefaultTableColumnModel;
import javax.swing.table.TableColumnModel;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.MutableTreeNode;
import javax.swing.tree.TreeNode;

public class DailyReportTreeTableModel extends TreeTableModel {
    public static class CategoryNode extends DefaultMutableTreeNode {
        TimeData.TimeCategory category;
        TimeData.TimeCode code;
        long minutes;

        @Override
        public void add(MutableTreeNode child) {
            super.add(child);

            // add minutes
            if (((TaskNode) child).minutes != null) {
                minutes += ((TaskNode) child).minutes;
            }
        }

        @Override
        public void remove(MutableTreeNode node) {
            super.remove(node);

            if (((TaskNode) node).minutes != null) {
                minutes -= ((TaskNode) node).minutes;
            }
        }
    }

    public static class TaskNode extends DefaultMutableTreeNode {
        Task task;
        Long minutes = null;
    }

    public DailyReportTreeTableModel(TreeNode rootNode) {
        super(rootNode, false);
    }


    @Override
    public Class<?> getColumnClass(final int columnIndex) {
        if (columnIndex == 1) {
            return long.class;
        }
        return String.class;
    }

    @Override
    public Object getColumnValue(final TreeNode node, final int column) {
        if (node instanceof CategoryNode categoryNode) {
            switch (column) {
                case 0:
                    return categoryNode.category.name + " - " + categoryNode.code.name;
                case 1:
                    return categoryNode.minutes;
            }
        }
        else if (node instanceof TaskNode taskNode) {
            switch (column) {
                case 0:
                    return taskNode.task.name;
                case 1:
                    return taskNode.minutes;
            }
        }
        return null;
    }

    @Override
    protected TableColumnModel createTableColumnModel() {
        TableColumnModel result = new DefaultTableColumnModel();
        result.addColumn(TableUtils.createColumn(0, "Description"));
        result.addColumn(TableUtils.createColumn(1, "Time"));
        return result;
    }
}
