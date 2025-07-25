package tree;

import data.Task;
import net.byteseek.swing.treetable.TreeTableHeaderRenderer;
import net.byteseek.swing.treetable.TreeTableModel;
import packets.DailyReportMessage;
import packets.TaskInfo;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.TreeNode;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

public class DailyReportTreeTable extends JTable {
    private MainFrame mainFrame;
    private DefaultMutableTreeNode rootNode;
    private TreeTableModel treeTableModel;
    private DefaultTreeModel treeModel;

    public DailyReportTreeTable(DailyReportMessage.DailyReport report) {
        update(report);
    }

    public void update(DailyReportMessage.DailyReport report) {
        rootNode = new DefaultMutableTreeNode();

        List<DailyReportTreeTableModel.CategoryNode> categoryNodes = new ArrayList<>();

        report.timesPerTimeEntry.forEach((timeEntry, time) -> {
            long minutes = TimeUnit.MILLISECONDS.toMinutes(time.toEpochMilli());

            DailyReportTreeTableModel.CategoryNode categoryNode = new DailyReportTreeTableModel.CategoryNode();
            categoryNode.category = timeEntry.category;
            categoryNode.code = timeEntry.code;
            categoryNode.minutes = minutes;

            categoryNodes.add(categoryNode);
        });

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

                // find the node for this session
                categoryNodes.stream()
                        .filter(categoryNode -> categoryNode.category.equals(session.timeEntry))
            }

        }
        for (DailyReportMessage.DailyReport.TimePair time : report.times) {

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
