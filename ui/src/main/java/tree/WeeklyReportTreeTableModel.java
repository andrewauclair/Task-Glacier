package tree;

import net.byteseek.swing.treetable.TableUtils;
import net.byteseek.swing.treetable.TreeTableModel;

import javax.swing.table.DefaultTableColumnModel;
import javax.swing.table.TableColumnModel;
import javax.swing.tree.MutableTreeNode;
import javax.swing.tree.TreeNode;

public class WeeklyReportTreeTableModel extends TreeTableModel {
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

    String[] dates = new String[7];

    public WeeklyReportTreeTableModel(TreeNode rootNode) {
        super(rootNode, false);
    }

    @Override
    public Class<?> getColumnClass(final int columnIndex) {
        if (columnIndex >= 1 && columnIndex < 8) {
            return long.class;
        }
        return String.class;
    }

    @Override
    public Object getColumnValue(final TreeNode node, final int column) {
        if (node instanceof WeeklyCategoryNode categoryNode) {
            switch (column) {
                case 0:
                    return categoryNode.category.name + " - " + categoryNode.code.name;
                case 8:
                    return categoryNode.minutes;
            }
            return categoryNode.minutesPerDay[column - 1];
        }
        else if (node instanceof WeeklyTaskNode taskNode) {
            switch (column) {
                case 0:
                    return taskNode.task.name;
                case 8:
                    return taskNode.minutes;
            }
            return taskNode.minutesPerDay[column - 1];
        }
        return null;
    }

    @Override
    protected TableColumnModel createTableColumnModel() {
        TableColumnModel result = new DefaultTableColumnModel();
        result.addColumn(TableUtils.createColumn(0, "Description"));


        for (int i = 0; i < 7; i++) {
            result.addColumn(TableUtils.createColumn(i + 1, ""));
        }
        result.addColumn(TableUtils.createColumn(8, "Time"));
        return result;
    }

    public void updateDateColumns(String[] dates) {
        TableColumnModel result = getTableColumnModel();
        for (int i = 0; i < 7; i++) {
            result.getColumn(i + 1).setHeaderValue(dates[i]);
        }
    }
}
