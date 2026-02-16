package tree;

import data.Task;
import data.TimeData;
import net.byteseek.swing.treetable.TreeTableModel;
import packets.DailyReportMessage;
import packets.TaskInfo;
import packets.WeeklyReport;
import tree.DailyReportTreeTableModel.CategoryNode;
import tree.DailyReportTreeTableModel.TaskNode;
import tree.DailyReportTreeTableModel.TotalCategoryNode;
import tree.WeeklyReportTreeTableModel.WeeklyCategoryNode;
import tree.WeeklyReportTreeTableModel.WeeklyTaskNode;
import tree.WeeklyReportTreeTableModel.WeeklyTotalCategoryNode;

import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.MutableTreeNode;
import javax.swing.tree.TreeNode;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import static taskglacier.MainFrame.mainFrame;

/*

TODO this works pretty well, but there's a big issue with it: task sessions changing category/code.

Right now I don't know how to handle this, or even to properly detect it. I'm just doing way too much mapping
already and I don't want more. I need to find a way to flatten this out. The goal is to not replace notes if I don't have to.
If the only thing that has changed since the last report update is more time passing, then the nodes should stay whwere they
are and expanded how they are. If tasks change parents, we can already detect that and handle it. But it's dealing with
session changes that's a nightmare.

I guess what I could do here is have the server track this instead, per task. Then when it generates a new report for the UI
it can notify the UI to expect session changes and rebuild the tree tables. That sounds much nicer than what I'm doing here
currently.

Perhaps there's also a way to flatten all of this out by restructuring the report packets to provide more context to build
the proper nodes. Instead of looking all of the information up and creating all these maps.

 */
public class ReportBuilder {
    private final DefaultMutableTreeNode rootNode;
    private final TreeTableModel treeTableModel;
    private final DefaultTreeModel treeModel;
    // we'll use this to decide which nodes to delete, if any, later
    Map<TimeData.TimeEntry, Set<Task>> tasks = new HashMap<>();
    // we'll use this to decide which nodes to delete, if any, later
    Map<TimeData.TimeEntry, Set<Task>> tasksThisUpdate = new HashMap<>();
    Map<Integer, Integer> parents = new HashMap<>();
    Map<TimeData.TimeCategory, TotalCategoryNode> totals = new HashMap<>();
    private Map<TimeData.TimeEntry, CategoryNode> categoryNodes = new HashMap<>();

    public ReportBuilder(DefaultMutableTreeNode rootNode, TreeTableModel treeTableModel, DefaultTreeModel treeModel) {
        this.rootNode = rootNode;
        this.treeTableModel = treeTableModel;
        this.treeModel = treeModel;

        treeTableModel.setNodeFilter(treeNode -> false);
    }

    public void updateForDailyReport(DailyReportMessage.DailyReport report) {
        pre(Collections.singletonList(report));
        update(report, 0);
        post(Collections.singletonList(report));
    }

    public void updateForWeeklyReport(WeeklyReport report) {
        String[] dates = new String[7];
        for (int i = 0; i < 7; i++) {
            DailyReportMessage.DailyReport dailyReport = report.reports[i];

            dates[i] = String.format("%d/%d/%d", dailyReport.month, dailyReport.day, dailyReport.year);
        }

        if (treeTableModel instanceof WeeklyReportTreeTableModel model) {
            model.updateDateColumns(dates);
        }

        List<DailyReportMessage.DailyReport> reports = new ArrayList<>();

        for (DailyReportMessage.DailyReport dailyReport : report.reports) {
            reports.add(dailyReport);
        }

        pre(reports);

        int index = 0;

        for (DailyReportMessage.DailyReport dailyReport : reports) {
            update(dailyReport, index);
            index++;
        }

        post(reports);
    }

    private void pre(List<DailyReportMessage.DailyReport> reports) {
        List<Integer> taskIDs = reports.stream()
                .flatMap(report -> report.times.stream())
                .map(timePair -> timePair.taskID)
                .toList();

        boolean parentsChanged = false;

        for (int taskID : taskIDs) {
            Task task = mainFrame.getTaskModel().getTask(taskID);

            Integer parent = parents.get(task.id);

            if (parent != null && parent.intValue() != task.parentID) {
                parentsChanged = true;
                break;
            }
        }

        // if parents have changed, let's throw out all the task nodes and start over
        if (parentsChanged) {
            System.out.println("Parents changed, delete all tasks from display");
            for (CategoryNode value : categoryNodes.values()) {
                Enumeration<TreeNode> children = value.children();
                while (children.hasMoreElements()) {
                    treeModel.removeNodeFromParent((MutableTreeNode) children.nextElement());
                }
            }
            tasks.clear();
        }

        parents.clear();

        for (int taskID : taskIDs) {
            Task task = mainFrame.getTaskModel().getTask(taskID);
            parents.put(task.id, task.parentID);
        }

        addNewCategoryNodes(reports);
        removeUnusedCategoryNodes(reports);

        // walk the entire tree and clear the times
        for (CategoryNode categoryNode : categoryNodes.values()) {
            categoryNode.minutes = 0;

            if (categoryNode instanceof WeeklyCategoryNode weeklyCategoryNode) {
                weeklyCategoryNode.minutesPerDay = new Long[7];
            }

            Enumeration<TreeNode> children = categoryNode.children();

            while (children.hasMoreElements()) {
                TaskNode taskNode = (TaskNode) children.nextElement();

                resetTaskTimes(taskNode);
            }
        }

        for (TotalCategoryNode totalCategoryNode : totals.values()) {
            totalCategoryNode.minutes = 0;

            if (totalCategoryNode instanceof WeeklyTotalCategoryNode weeklyTotalNode) {
                weeklyTotalNode.minutesPerDay = new Long[7];
                weeklyTotalNode.minutes = 0;
            }
        }
        tasksThisUpdate.clear();
    }

    private void resetTaskTimes(TaskNode node) {
        node.setMinutes(null);

        if (node instanceof WeeklyTaskNode weeklyTaskNode) {
            weeklyTaskNode.minutesPerDay = new Long[7];
        }

        Enumeration<TreeNode> children = node.children();

        while (children.hasMoreElements()) {
            resetTaskTimes((TaskNode) children.nextElement());
        }
    }

    // remove any tasks that are no longer part of the report
    // add up task times for parents
    private void post(List<DailyReportMessage.DailyReport> reports) {
        tasks.forEach((timeEntry, tasks1) -> {
            Set<Task> thisUpdate = tasksThisUpdate.get(timeEntry);

            if (thisUpdate != null) {
                CategoryNode categoryNode = categoryNodes.get(timeEntry);

                for (Task task : tasks1) {
                    if (!thisUpdate.contains(task)) {
                        // remove
                        treeModel.removeNodeFromParent(findOrCreateTaskNode(categoryNode, task));
                    }
                }
            }
        });

        this.tasks = tasksThisUpdate;

        // add up task times for parents
        categoryNodes.forEach((timeEntry, categoryNode) -> {
            fillTimesForParents(categoryNode);
        });
    }

    private void fillTimesForParents(DefaultMutableTreeNode parent) {
        Enumeration<TreeNode> children = parent.children();

        while (children.hasMoreElements()) {
            DefaultMutableTreeNode treeNode = (DefaultMutableTreeNode) children.nextElement();

            fillTimesForParents(treeNode);

            if (treeNode instanceof TaskNode taskNode) {
                if (taskNode instanceof WeeklyTaskNode weeklyTask) {
                    for (int i = 0; i < 7; i++) {
                        if (weeklyTask.minutesPerDay[i] == null) {
                            weeklyTask.minutesPerDay[i] = totalMinutesForTaskChildren(weeklyTask, i);
                        }
                    }
                }
                else {
                    // make sure to only set the minutes for tasks that do not have minutes
                    if (taskNode.getMinutes() == null) {
                        taskNode.setMinutes(totalMinutesForTaskChildren(taskNode, 0));
                    }
                }
            }
        }
    }

    private Long totalMinutesForTaskChildren(TaskNode node, int day) {
        Long minutes = null;

        Enumeration<TreeNode> children = node.children();

        while (children.hasMoreElements()) {
            TreeNode treeNode = children.nextElement();

            if (treeNode instanceof TaskNode taskNode) {
                if (taskNode instanceof WeeklyTaskNode weeklyTask) {
                    if (weeklyTask.minutesPerDay[day] != null) {
                        if (minutes == null) {
                            minutes = 0L;
                        }
                        minutes += weeklyTask.minutesPerDay[day];
                    }
                    Long childrenMinutes = totalMinutesForTaskChildren(weeklyTask, day);

                    if (childrenMinutes != null) {
                        if (minutes == null) {
                            minutes = 0L;
                        }
                        minutes += childrenMinutes;
                    }
                }
                else {
                    if (taskNode.getMinutes() != null) {
                        if (minutes == null) {
                            minutes = 0L;
                        }
                        minutes += taskNode.getMinutes();
                    }

                    Long childrenMinutes = totalMinutesForTaskChildren(taskNode, 0);

                    if (childrenMinutes != null) {
                        if (minutes == null) {
                            minutes = 0L;
                        }
                        minutes += childrenMinutes;
                    }
                }
            }
        }

        return minutes;
    }

    private void update(DailyReportMessage.DailyReport report, int index) {
        Set<Integer> taskIDs = report.times.stream()
                .map(timePair -> timePair.taskID)
                .collect(Collectors.toSet());

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
                    CategoryNode categoryNode = categoryNodes.get(timeEntry);
                    categoryNode.minutes += minutes;

                    if (categoryNode instanceof WeeklyCategoryNode weeklyCategoryNode) {
                        if (weeklyCategoryNode.minutesPerDay[index] == null) {
                            weeklyCategoryNode.minutesPerDay[index] = 0L;
                        }
                        weeklyCategoryNode.minutesPerDay[index] += minutes;
                    }
                    Set<Task> orDefault = tasks.getOrDefault(timeEntry, new HashSet<>());
                    tasksThisUpdate.put(timeEntry, orDefault);
                    orDefault.add(task);

                    TaskNode taskNode = findOrCreateTaskNode(categoryNode, task);
                    if (taskNode instanceof WeeklyTaskNode weeklyTask) {
                        if (weeklyTask.minutesPerDay[index] == null) {
                            weeklyTask.minutesPerDay[index] = 0L;
                        }
                        weeklyTask.minutesPerDay[index] += minutes;
                        WeeklyTotalCategoryNode total = (WeeklyTotalCategoryNode) totals.get(timeEntry.category);
                        if (total.minutesPerDay[index] == null) {
                            total.minutesPerDay[index] = 0L;
                        }
                        total.minutesPerDay[index] += minutes;
                    }
                    else {
                        taskNode.setMinutes(minutes);
                    }

                    totals.get(timeEntry.category).minutes += minutes;
                }
            }
        }
    }

    private TaskNode findOrCreateTaskNode(CategoryNode categoryNode, Task task) {
        // first, jump to the highest non-0 parent
        List<Task> history = new ArrayList<>();
        history.add(task);

        Task current = task;

        while (current.parent != null) {
            history.add(0, current.parent);
            current = current.parent;
        }

        DefaultMutableTreeNode node = categoryNode;
        for (Task task1 : history) {
            Enumeration<TreeNode> children = node.children();
            boolean found = false;
            while (children.hasMoreElements()) {
                DefaultMutableTreeNode child = (DefaultMutableTreeNode) children.nextElement();

                if (child instanceof TaskNode taskNode) {
                    if (task1 == taskNode.task) {
                        // found it, loop again
                        node = child;
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                // didn't find it. create it
                TaskNode newNode = treeTableModel instanceof DailyReportTreeTableModel ? new TaskNode() : new WeeklyTaskNode();
                newNode.task = task1;
                treeModel.insertNodeInto(newNode, node, 0);
                node = newNode;
            }
        }
        return (TaskNode) node;
    }

    private void addNewCategoryNodes(List<DailyReportMessage.DailyReport> reports) {
        for (TotalCategoryNode value : totals.values()) {
            treeModel.removeNodeFromParent(value);
        }
        totals.clear();

        reports.forEach(report -> {
            report.timesPerTimeEntry.forEach((timeEntry, time) -> {
                long minutes = TimeUnit.MILLISECONDS.toMinutes(time.toEpochMilli());

                if (!totals.containsKey(timeEntry.category)) {
                    TotalCategoryNode total = treeTableModel instanceof DailyReportTreeTableModel ? new TotalCategoryNode() : new WeeklyTotalCategoryNode();
                    total.category = timeEntry.category;
                    totals.put(timeEntry.category, total);
                    treeModel.insertNodeInto(total, rootNode, 0);
                }

                // search for it first
                if (!categoryNodes.containsKey(timeEntry)) {
                    CategoryNode categoryNode = treeTableModel instanceof DailyReportTreeTableModel ? new CategoryNode() : new WeeklyCategoryNode();
                    categoryNode.category = timeEntry.category;
                    categoryNode.code = timeEntry.code;
                    categoryNode.minutes = minutes;

                    categoryNode.setAllowsChildren(true);

                    categoryNodes.put(timeEntry, categoryNode);

                    treeModel.insertNodeInto(categoryNode, rootNode, 0);
                }
            });
        });
    }

    private void removeUnusedCategoryNodes(List<DailyReportMessage.DailyReport> reports) {
        List<TimeData.TimeEntry> timeEntries = reports.stream()
                .flatMap(report -> report.timesPerTimeEntry.keySet().stream())
                .toList();

        var iterator = categoryNodes.entrySet().iterator();

        while (iterator.hasNext()) {
            var next = iterator.next();

            if (!timeEntries.contains(next.getKey())) {
                treeModel.removeNodeFromParent(next.getValue());
                iterator.remove();
            }
        }
    }
}
