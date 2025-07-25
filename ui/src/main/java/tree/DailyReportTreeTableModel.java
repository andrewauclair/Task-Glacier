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
import javax.swing.tree.TreeNode;

public class DailyReportTreeTableModel extends TreeTableModel {
    public static class CategoryNode extends DefaultMutableTreeTableNode {
        TimeData.TimeCategory category;
        TimeData.TimeCode code;
        long minutes;

        @Override
        public void add(MutableTreeTableNode child) {
            super.add(child);

            // add minutes
            minutes += ((TaskNode) child).minutes;
        }

        @Override
        public void remove(MutableTreeTableNode node) {
            super.remove(node);

            minutes -= ((TaskNode) node).minutes;
        }
    }

    public static class TaskNode extends DefaultMutableTreeTableNode {
        Task task;
        long minutes;
    }

    public DailyReportTreeTableModel(TreeNode rootNode) {
        super(rootNode);
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
