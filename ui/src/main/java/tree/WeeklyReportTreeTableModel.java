package tree;

import net.byteseek.swing.treetable.TableUtils;
import net.byteseek.swing.treetable.TreeTableModel;

import javax.swing.table.DefaultTableColumnModel;
import javax.swing.table.TableColumnModel;
import javax.swing.tree.MutableTreeNode;
import javax.swing.tree.TreeNode;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.List;

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

        public Long getMinutes() {
            boolean anyExist = Arrays.stream(minutesPerDay).anyMatch(aLong -> aLong != null);

            if (!anyExist) {
                return null;
            }
            long total = 0;
            for (int i = 0; i < 7; i++) {
                if (minutesPerDay[i] != null) {
                    total += minutesPerDay[i];
                }
            }
            return total;
        }
    }

    public static class WeeklyTotalCategoryNode extends DailyReportTreeTableModel.TotalCategoryNode {
        Long[] minutesPerDay = new Long[7];
    }

    String[] dates = new String[7];

    public WeeklyReportTreeTableModel(TreeNode rootNode) {
        super(rootNode, false);

        setGroupingComparator((o1, o2) -> {
            // totals nodes are always less than
            if (o1 instanceof WeeklyTotalCategoryNode) {
                if (o2 instanceof WeeklyTotalCategoryNode) {
                    return 0;
                } else {
                    return 1;
                }
            } else if (o2 instanceof WeeklyTotalCategoryNode) {
                return -1;
            }

            long minutes1 = 0;
            long minutes2 = 0;

            if (o1 instanceof WeeklyCategoryNode categoryNode) {
                minutes1 = categoryNode.minutes;
            } else if (o1 instanceof WeeklyTaskNode taskNode) {
                minutes1 = taskNode.getMinutes();
            }

            if (o2 instanceof WeeklyCategoryNode categoryNode) {
                minutes2 = categoryNode.minutes;
            } else if (o2 instanceof WeeklyTaskNode taskNode) {
                minutes2 = taskNode.getMinutes();
            }

            return Long.compare(minutes2, minutes1);
        });
    }

    public int getFirstTotalsRowIndex() {
        Enumeration<? extends TreeNode> child = rootNode.children();

        List<Integer> rows = new ArrayList<>();

        while (child.hasMoreElements()) {
            TreeNode node = child.nextElement();

            if (node instanceof WeeklyTotalCategoryNode) {
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
                    return taskNode.getMinutes();
            }
            return taskNode.minutesPerDay[column - 1];
        }
        else if (node instanceof WeeklyTotalCategoryNode totalNode) {
            switch (column) {
                case 0:
                    return totalNode.category.name;
                case 8:
                    return totalNode.minutes;
            }
            return totalNode.minutesPerDay[column - 1];
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
