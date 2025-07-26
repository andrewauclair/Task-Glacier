package tree;

import data.Task;
import data.TimeData;
import net.byteseek.swing.treetable.TreeTableHeaderRenderer;
import net.byteseek.swing.treetable.TreeTableModel;
import packets.DailyReportMessage;
import packets.TaskInfo;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.TreeNode;
import java.awt.*;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.TimeUnit;

import static taskglacier.MainFrame.mainFrame;

public class DailyReportTreeTable extends JTable {
    private final DefaultMutableTreeNode rootNode = new DefaultMutableTreeNode();
    private final TreeTableModel treeTableModel;
    private final DefaultTreeModel treeModel;

    private Map<TimeData.TimeEntry, DailyReportTreeTableModel.CategoryNode> categoryNodes = new HashMap<>();

    public DailyReportTreeTable() {
        treeModel = createTreeModel(rootNode);
        treeTableModel = createTreeTableModel(rootNode);

        getColumnModel().getColumn(1).setCellRenderer(new ElapsedTimeCellRenderer());

        setIntercellSpacing(new Dimension(0, 0));
    }

    public void update(DailyReportMessage.DailyReport report) {
        addNewCategoryNodes(report);
        removeUnusedCategoryNodes(report);

        List<Integer> taskIDs = report.times.stream()
                .map(timePair -> timePair.taskID)
                .toList();

        for (Integer taskID : taskIDs) {
            List<DailyReportMessage.DailyReport.TimePair> pairs = report.times.stream()
                    .filter(timePair -> timePair.taskID == taskID)
                    .toList();

            Task task = mainFrame.getTaskModel().getTask(taskID);

            for (DailyReportMessage.DailyReport.TimePair pair : pairs) {
                TaskInfo.Session session = task.sessions.get(pair.index);

                Instant stopTime = session.stopTime.orElseGet(() -> report.time);
                Instant instant = stopTime.minusMillis(session.startTime.toEpochMilli());
                long minutes = TimeUnit.MILLISECONDS.toMinutes(instant.toEpochMilli());

                for (TimeData.TimeEntry timeEntry : session.timeEntry) {
                    Optional<DailyReportTreeTableModel.CategoryNode> list = categoryNodes.values().stream()
                            .filter(categoryNode -> categoryNode.category.equals(timeEntry.category) && categoryNode.code.equals(timeEntry.code))
                            .findFirst();

                    if (list.isPresent()) {
                        DailyReportTreeTableModel.TaskNode child = new DailyReportTreeTableModel.TaskNode();
                        child.task = task;
                        child.minutes = minutes;
                        list.get().add(child);
                    }
                }
            }
        }
    }

    private void addNewCategoryNodes(DailyReportMessage.DailyReport report) {
        report.timesPerTimeEntry.forEach((timeEntry, time) -> {
            long minutes = TimeUnit.MILLISECONDS.toMinutes(time.toEpochMilli());

            // search for it first
            if (!categoryNodes.containsKey(timeEntry)) {
                DailyReportTreeTableModel.CategoryNode categoryNode = new DailyReportTreeTableModel.CategoryNode();
                categoryNode.category = timeEntry.category;
                categoryNode.code = timeEntry.code;
                categoryNode.minutes = minutes;

                categoryNodes.put(timeEntry, categoryNode);
                rootNode.add(categoryNode);
            }
        });
    }

    private void removeUnusedCategoryNodes(DailyReportMessage.DailyReport report) {
        var iterator = categoryNodes.entrySet().iterator();

        while (iterator.hasNext()) {
            var next = iterator.next();

            if (!report.timesPerTimeEntry.keySet().contains(next.getKey())) {
                rootNode.remove(next.getValue());
                iterator.remove();
            }
        }
    }

    private DefaultTreeModel createTreeModel(TreeNode rootNode) {
        DefaultTreeModel model = new DefaultTreeModel(rootNode);
        model.addTreeModelListener(treeTableModel);
        return model;
    }

    private TreeTableModel createTreeTableModel(TreeNode rootNode) {
        TreeTableModel localTreeTableModel = new DailyReportTreeTableModel(rootNode);

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
