package tree;

import data.Task;
import data.TimeData;
import net.byteseek.swing.treetable.TreeTableModel;
import net.byteseek.swing.treetable.TreeUtils;
import org.jdesktop.swingx.treetable.DefaultMutableTreeTableNode;
import org.jdesktop.swingx.treetable.MutableTreeTableNode;

import javax.swing.table.TableColumnModel;
import javax.swing.tree.TreeNode;

public class DailyReportTreeTableModel extends TreeTableModel {
    private static class CategoryNode extends DefaultMutableTreeTableNode {
        TimeData.TimeCategory category;
        TimeData.TimeCode code;
        int minutes;

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

    private static class TaskNode extends DefaultMutableTreeTableNode {
        Task task;
        int minutes;
    }

    public DailyReportTreeTableModel(TreeNode rootNode) {
        super(rootNode);
    }

    @Override
    public Class<?> getColumnClass(final int columnIndex) {
        return String.class;
    }

    @Override
    public Object getColumnValue(final TreeNode node, final int column) {
        if (node instanceof CategoryNode categoryNode) {
            switch (column) {
                case 0:
                    return categoryNode.category.name;
                case 1:
                    return categoryNode.code.name;
                case 2:
                    return categoryNode.minutes;
            }
        }
        else if (node instanceof TaskNode taskNode) {
            switch (column) {
                case 2:
                    return taskNode.minutes;
            }
        }
        return null;
    }

    @Override
    protected TableColumnModel createTableColumnModel() {
        return null;
    }
}
