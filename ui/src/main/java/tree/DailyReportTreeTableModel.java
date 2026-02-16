package tree;

import data.Task;
import data.TimeData;
import net.byteseek.swing.treetable.TableUtils;
import net.byteseek.swing.treetable.TreeTableModel;

import javax.swing.table.DefaultTableColumnModel;
import javax.swing.table.TableColumnModel;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.MutableTreeNode;
import javax.swing.tree.TreeNode;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;

public class DailyReportTreeTableModel extends TreeTableModel {
    public DailyReportTreeTableModel(TreeNode rootNode) {
        super(rootNode, false);

        setGroupingComparator((o1, o2) -> {
            // totals nodes are always less than
            if (o1 instanceof TotalCategoryNode a) {
                if (o2 instanceof TotalCategoryNode b) {
                    return Integer.compare(a.category.id, b.category.id);
                }
                else {
                    return 1;
                }
            }
            else if (o2 instanceof TotalCategoryNode) {
                return -1;
            }

            if (o1 instanceof CategoryNode categoryNode1 &&
                    o2 instanceof CategoryNode categoryNode2 && categoryNode1.category.id != categoryNode2.category.id) {
                return Integer.compare(categoryNode1.category.id, categoryNode2.category.id);
            }

            long minutes1 = 0;
            long minutes2 = 0;

            if (o1 instanceof CategoryNode categoryNode) {
                minutes1 = categoryNode.minutes;
            }
            else if (o1 instanceof TaskNode taskNode) {
                minutes1 = taskNode.minutes == null ? 0 : taskNode.minutes;
            }

            if (o2 instanceof CategoryNode categoryNode) {
                minutes2 = categoryNode.minutes;
            }
            else if (o2 instanceof TaskNode taskNode) {
                minutes2 = taskNode.minutes == null ? 0 : taskNode.minutes;
            }

            return Long.compare(minutes2, minutes1);
        });
    }

    public int getFirstTotalsRowIndex() {
        Enumeration<? extends TreeNode> child = rootNode.children();

        List<Integer> rows = new ArrayList<>();

        while (child.hasMoreElements()) {
            TreeNode node = child.nextElement();

            if (node instanceof TotalCategoryNode) {
                rows.add(getTable().convertRowIndexToView(getModelIndexForTreeNode(node)));
            }
        }

        rows.sort(Integer::compare);

        if (rows.isEmpty()) {
            return 0;
        }
        return rows.get(0);
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
        else if (node instanceof TotalCategoryNode total) {
            switch (column) {
                case 0:
                    return total.category.name;
                case 1:
                    return total.minutes;
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

        public long minutesChildren = 0;

        private Long minutes = null;

        public Long getMinutes() {
            return minutes;
        }

        public void setMinutes(Long minutes) {
            this.minutes = minutes;
        }
    }

    public static class TotalCategoryNode extends DefaultMutableTreeNode {
        TimeData.TimeCategory category;
        long minutes;
    }
}
